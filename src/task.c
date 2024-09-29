#define _XOPEN_SOURCE 700  // For strptime

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ncurses.h>
#include <time.h>
#include <pthread.h>
#include "todo.h"

const char *recurrence_strings[] = {
    "none",
    "daily",
    "weekly",
    "biweekly",
    "monthly",
    "yearly"
};

extern Action action_stack[];
extern int action_count;
extern pthread_mutex_t task_mutex;

void get_input(char *buffer, int size, const char *prompt) {
    mvprintw(LINES - 2, 0, "%s", prompt);
    echo();
    getnstr(buffer, size - 1);
    noecho();
    buffer[size - 1] = '\0';
    refresh();
}

void ensure_capacity(Task **tasks, int *capacity, int needed) {
    if (needed > *capacity) {
        *capacity = needed * 2;
        Task *temp = realloc(*tasks, (*capacity) * sizeof(Task));
        if (temp == NULL) {
            handle_error("Error reallocating memory for tasks.");
            free(*tasks);  // Free existing memory before exiting
            exit(1);
        }
        *tasks = temp;
    }
}

// Function to update task IDs after sorting or deletion
void update_task_ids(Task *tasks, int count) {
    for (int i = 0; i < count; i++) {
        tasks[i].id = i + 1;
    }
}

// Comparator functions for sorting by priority, due date
int compare_priority_desc(const void *a, const void *b) {
    Task *taskA = (Task *)a;
    Task *taskB = (Task *)b;
    return taskB->priority - taskA->priority;  // Higher priority first
}

int compare_priority_asc(const void *a, const void *b) {
    Task *taskA = (Task *)a;
    Task *taskB = (Task *)b;
    return taskA->priority - taskB->priority;  // Lower priority first
}

int compare_due_date_desc(const void *a, const void *b) {
    Task *taskA = (Task *)a;
    Task *taskB = (Task *)b;

    if (strcmp(taskA->due_date, "N/A") == 0) return 1;  // Push "N/A" (no due date) to the end
    if (strcmp(taskB->due_date, "N/A") == 0) return -1;

    return strcmp(taskA->due_date, taskB->due_date);  // Closest date first
}

int compare_due_date_asc(const void *a, const void *b) {
    Task *taskA = (Task *)a;
    Task *taskB = (Task *)b;

    if (strcmp(taskA->due_date, "N/A") == 0) return 1;  // Push "N/A" (no due date) to the end
    if (strcmp(taskB->due_date, "N/A") == 0) return -1;

    return strcmp(taskB->due_date, taskA->due_date);  // Latest date first
}

// Function to handle sorting with ascending/descending toggling
void sort_tasks(Task *tasks, int count, char sort_type, bool ascending) {
    switch (sort_type) {
        case 'p':  // Sort by priority
            if (ascending) {
                qsort(tasks, count, sizeof(Task), compare_priority_asc);
            } else {
                qsort(tasks, count, sizeof(Task), compare_priority_desc);
            }
            break;
        case 'd':  // Sort by due date
            if (ascending) {
                qsort(tasks, count, sizeof(Task), compare_due_date_desc);
            } else {
                qsort(tasks, count, sizeof(Task), compare_due_date_asc);
            }
            break;
    }
    update_task_ids(tasks, count);
}

void init_ncurses() {
    if (initscr() == NULL) {
        fprintf(stderr, "Error initializing ncurses.\n");
        exit(1);
    }
    if (has_colors() == FALSE) {
        endwin();
        fprintf(stderr, "Your terminal does not support color.\n");
        exit(1);
    }
    start_color();
    noecho();
    cbreak();
    keypad(stdscr, TRUE);
    curs_set(0);
    refresh();
    // Initialize color pairs
    init_pair(1, COLOR_RED, COLOR_BLACK);     // Red for overdue tasks
    init_pair(2, COLOR_YELLOW, COLOR_BLACK);  // Yellow for due soon tasks
    init_pair(3, COLOR_GREEN, COLOR_BLACK);   // Priority 1
    init_pair(4, COLOR_BLUE, COLOR_BLACK);    // Priority 2
    init_pair(5, COLOR_CYAN, COLOR_BLACK);    // Priority 3
    init_pair(6, COLOR_MAGENTA, COLOR_BLACK); // Priority 4
    init_pair(7, COLOR_WHITE, COLOR_BLACK);   // Priority 5
}

void cleanup_ncurses() {
    endwin();
}

void toggle_task_completion(Task *task) {
    // Push action onto the stack
    if (action_count < MAX_ACTIONS) {
        action_stack[action_count].type = ACTION_COMPLETE;
        action_stack[action_count].task = *task;
        action_stack[action_count].index = task->id - 1;
        action_count++;
    }

    // Toggle the completion status (1 for completed, 0 for not completed)
    task->completed = !task->completed;

    // If the task is recurring, update the due date when completed
    if (task->completed && task->recurrence != RECURRENCE_NONE) {
        update_task_recurrence(task);
    }

    // Log the action
    log_message("Task completion status toggled.");
}

void update_task_recurrence(Task *task) {
    if (task->recurrence == RECURRENCE_NONE) {
        return;  // No recurrence, nothing to update
    }

    struct tm due_date = {0};

    if (!parse_date(task->due_date, &due_date)) {
        // Invalid or uninitialized due date, do nothing
        return;
    }

    // Adjust the due date based on recurrence type
    switch (task->recurrence) {
        case RECURRENCE_DAILY:
            due_date.tm_mday += 1;
            break;
        case RECURRENCE_WEEKLY:
            due_date.tm_mday += 7;
            break;
        case RECURRENCE_BIWEEKLY:
            due_date.tm_mday += 14;
            break;
        case RECURRENCE_MONTHLY:
            due_date.tm_mon += 1;
            break;
        case RECURRENCE_YEARLY:
            due_date.tm_year += 1;
            break;
        default:
            break;
    }

    // Normalize the date
    mktime(&due_date);

    // Update the due date field
    format_date(task->due_date, MAX_DATE_LEN, &due_date);
}

int is_task_overdue(Task task) {
    if (strcmp(task.due_date, "N/A") == 0) {
        return 0;  // No due date, so not overdue
    }

    // Get the current time
    time_t t = time(NULL);
    struct tm current_time = *localtime(&t);
    struct tm due_date = {0};

    if (!parse_date(task.due_date, &due_date)) {
        return 0;  // Invalid due date format
    }

    // Compare the due date with the current date
    return mktime(&current_time) > mktime(&due_date);
}

int is_task_due_soon(Task task) {
    if (strcmp(task.due_date, "N/A") == 0) return 0;

    time_t t = time(NULL);
    struct tm current_time = *localtime(&t);
    struct tm due_date = {0};

    if (!parse_date(task.due_date, &due_date)) {
        return 0;  // Invalid due date format
    }

    double seconds_difference = difftime(mktime(&due_date), mktime(&current_time));
    return (seconds_difference <= 86400 && seconds_difference >= 0);  // Task due in the next 24 hours
}

void add_task(Task **tasks, int *count, int *capacity, const char *title, const char *category, const char *due_date_str, RecurrenceType recurrence, int priority) {
    ensure_capacity(tasks, capacity, *count + 1);

    char temp_title[MAX_TITLE_LEN];
    char temp_category[MAX_CATEGORY_LEN];
    char temp_due_date[MAX_DATE_LEN];
    char temp_recurrence[MAX_RECURRENCE_LEN];
    int temp_priority;

    // Title input with retry if blank
    while (1) {
        get_input_and_clear(temp_title, MAX_TITLE_LEN, "Enter task title (cannot be empty): ");
        if (strlen(temp_title) > 0) break;
        mvprintw(LINES - 2, 0, "Task title cannot be empty. Please try again.");
        refresh();
        getch();
    }

    // Category input with retry if blank
    while (1) {
        get_input_and_clear(temp_category, MAX_CATEGORY_LEN, "Enter category (cannot be empty): ");
        if (strlen(temp_category) > 0) break;
        mvprintw(LINES - 2, 0, "Category cannot be empty. Please try again.");
        refresh();
        getch();
    }

    // Due date input with validation
    while (1) {
        get_input_and_clear(temp_due_date, MAX_DATE_LEN, "Enter due date (YYYY-MM-DD) or leave blank for N/A: ");
        if (strlen(temp_due_date) == 0) {
            strncpy(temp_due_date, "N/A", MAX_DATE_LEN - 1);
            temp_due_date[MAX_DATE_LEN - 1] = '\0';
            break;
        } else if (parse_date(temp_due_date, &(struct tm){0})) {
            break;
        } else {
            mvprintw(LINES - 2, 0, "Invalid date format. Please try again.");
            refresh();
            getch();
        }
    }

    // Recurrence input with validation
    while (1) {
        get_input_and_clear(temp_recurrence, MAX_RECURRENCE_LEN, "Enter recurrence (none, daily, weekly, biweekly, monthly, yearly): ");
        RecurrenceType rec = parse_recurrence(temp_recurrence);
        if (rec != -1) {
            recurrence = rec;
            break;
        } else {
            mvprintw(LINES - 2, 0, "Invalid recurrence. Please try again.");
            refresh();
            getch();
        }
    }

    // Priority input with validation
    while (1) {
        mvprintw(LINES - 2, 0, "Enter priority (1-5): ");
        clrtoeol();
        echo();
        char priority_input[3];
        getnstr(priority_input, sizeof(priority_input) - 1);
        noecho();
        temp_priority = atoi(priority_input);
        if (temp_priority >= 1 && temp_priority <= 5) {
            break;
        } else {
            mvprintw(LINES - 2, 0, "Invalid priority. Please enter a value between 1 and 5.");
            refresh();
            getch();
        }
    }

    // Save the task using validated inputs
    (*tasks)[*count].id = *count + 1;
    strncpy((*tasks)[*count].title, temp_title, MAX_TITLE_LEN - 1);
    (*tasks)[*count].title[MAX_TITLE_LEN - 1] = '\0';
    strncpy((*tasks)[*count].category, temp_category, MAX_CATEGORY_LEN - 1);
    (*tasks)[*count].category[MAX_CATEGORY_LEN - 1] = '\0';
    strncpy((*tasks)[*count].due_date, temp_due_date, MAX_DATE_LEN - 1);
    (*tasks)[*count].due_date[MAX_DATE_LEN - 1] = '\0';
    (*tasks)[*count].recurrence = recurrence;
    (*tasks)[*count].priority = temp_priority;
    (*tasks)[*count].completed = 0;

    // Push action onto the stack
    if (action_count < MAX_ACTIONS) {
        action_stack[action_count].type = ACTION_ADD;
        action_stack[action_count].task = (*tasks)[*count];
        action_stack[action_count].index = *count;
        action_count++;
    }

    (*count)++;

    mvprintw(LINES - 2, 0, "Task added successfully! Press any key...");
    clrtoeol();
    refresh();
    getch();
    log_message("Task added.");
}

void remove_task(Task **tasks, int *count, int index) {
    if (index < 0 || index >= *count) {
        // Index out of bounds
        return;
    }

    for (int i = index; i < *count - 1; i++) {
        (*tasks)[i] = (*tasks)[i + 1];  // Shift tasks down
    }
    (*count)--;
}

void edit_task(Task *task) {
    char title[MAX_TITLE_LEN];
    char category[MAX_CATEGORY_LEN];
    char due_date[MAX_DATE_LEN];
    char recurrence_input[MAX_RECURRENCE_LEN];
    int priority;

    // Push action onto the stack
    if (action_count < MAX_ACTIONS) {
        action_stack[action_count].type = ACTION_EDIT;
        action_stack[action_count].task = *task;
        action_stack[action_count].index = task->id - 1;
        action_count++;
    }

    // Get input for task title
    get_input_and_clear(title, MAX_TITLE_LEN, "Edit task title (leave blank to keep current): ");
    if (strlen(title) > 0) {
        strncpy(task->title, title, MAX_TITLE_LEN - 1);
        task->title[MAX_TITLE_LEN - 1] = '\0';  // Ensure null termination
    }

    // Get input for category
    get_input_and_clear(category, MAX_CATEGORY_LEN, "Edit category (leave blank to keep current): ");
    if (strlen(category) > 0) {
        strncpy(task->category, category, MAX_CATEGORY_LEN - 1);
        task->category[MAX_CATEGORY_LEN - 1] = '\0';
    }

    // Get input for due date
    while (1) {
        get_input_and_clear(due_date, MAX_DATE_LEN, "Edit due date (YYYY-MM-DD, leave blank to keep current): ");
        if (strlen(due_date) == 0) {
            break;
        } else if (parse_date(due_date, &(struct tm){0})) {
            strncpy(task->due_date, due_date, MAX_DATE_LEN - 1);
            task->due_date[MAX_DATE_LEN - 1] = '\0';
            break;
        } else {
            mvprintw(LINES - 2, 0, "Invalid date format. Please try again.");
            refresh();
            getch();
        }
    }

    // Get input for recurrence
    while (1) {
        get_input_and_clear(recurrence_input, MAX_RECURRENCE_LEN, "Edit recurrence (none, daily, weekly, biweekly, monthly, yearly, leave blank to keep current): ");
        if (strlen(recurrence_input) == 0) {
            break;
        } else {
            RecurrenceType rec = parse_recurrence(recurrence_input);
            if (rec != -1) {
                task->recurrence = rec;
                break;
            } else {
                mvprintw(LINES - 2, 0, "Invalid recurrence. Please try again.");
                refresh();
                getch();
            }
        }
    }

    // Get input for priority
    mvprintw(LINES - 2, 0, "Edit priority (1-5, leave blank to keep current): ");
    clrtoeol();
    echo();
    char priority_input[3];  // Input for priority as a string
    getnstr(priority_input, sizeof(priority_input) - 1);
    noecho();

    if (strlen(priority_input) > 0) {
        priority = atoi(priority_input);
        if (priority >= 1 && priority <= 5) {
            task->priority = priority;
        }
    }

    mvprintw(LINES - 2, 0, "Task edited successfully! Press any key...");
    clrtoeol();
    refresh();
    getch();
    log_message("Task edited.");
}

void search_task(Task *tasks, int count, int *selected_task) {
    char search_query[MAX_TITLE_LEN];

    // Prompt the user to enter the search query (event name)
    get_input_and_clear(search_query, MAX_TITLE_LEN, "Enter event name to search: ");
    
    // Search through tasks
    for (int i = 0; i < count; i++) {
        if (strstr(tasks[i].title, search_query) != NULL) {
            *selected_task = i;  // Set selected task to the found event
            return;  // Exit once the event is found and selected
        }
    }

    // If no event is found
    mvprintw(LINES - 2, 0, "Event not found. Press any key to continue.");
    clrtoeol();
    refresh();
    getch();  // Wait for user to acknowledge
}


void load_tasks(Task **tasks, int *count, int *capacity) {
    char *file_path = get_database_path();
    FILE *file = fopen(file_path, "r");

    *capacity = 10;
    *tasks = malloc((*capacity) * sizeof(Task));
    if (*tasks == NULL) {
        handle_error("Error allocating memory for tasks.");
        exit(1);
    }

    if (file == NULL) {
        handle_error("Tasks file not found. Starting with an empty task list.");
        return;
    }

    while (!feof(file)) {
        ensure_capacity(tasks, capacity, *count + 1);
        Task *task = &(*tasks)[*count];
        char recurrence_str[MAX_RECURRENCE_LEN];
        if (fscanf(file, "%d\t%[^\t]\t%[^\t]\t%d\t%d\t%s\t%s\n", 
                   &task->id, task->title, task->category,
                   &task->priority, &task->completed,
                   task->due_date, recurrence_str) == 7) {
            task->recurrence = parse_recurrence(recurrence_str);
            (*count)++;
        } else {
            // Skip malformed lines
            fscanf(file, "%*[^\n]\n");
        }
    }

    fclose(file);
}

typedef struct {
    Task *tasks;
    int count;
} SaveArgs;

void *save_tasks_async(void *arg) {
    SaveArgs *args = (SaveArgs *)arg;
    pthread_mutex_lock(&task_mutex);
    save_tasks(args->tasks, args->count);
    free(args->tasks);
    free(args);
    pthread_mutex_unlock(&task_mutex);
    return NULL;
}

void trigger_save_tasks(Task *tasks, int count) {
    pthread_t save_thread;
    SaveArgs *args = malloc(sizeof(SaveArgs));
    if (args == NULL) {
        handle_error("Error allocating memory for saving tasks.");
        return;
    }
    args->tasks = malloc(count * sizeof(Task));
    if (args->tasks == NULL) {
        handle_error("Error allocating memory for saving tasks.");
        free(args);
        return;
    }
    memcpy(args->tasks, tasks, count * sizeof(Task));
    args->count = count;

    if (pthread_create(&save_thread, NULL, save_tasks_async, args) != 0) {
        handle_error("Error creating thread for saving tasks.");
        free(args->tasks);
        free(args);
    } else {
        pthread_detach(save_thread);
    }
}

void save_tasks(Task *tasks, int count) {
    char *file_path = get_database_path();
    FILE *file = fopen(file_path, "w");
    if (file == NULL) {
        // Handle file creation failure
        handle_error("Error: Could not open file for saving tasks.");
        return;
    }

    for (int i = 0; i < count; i++) {
        fprintf(file, "%d\t%s\t%s\t%d\t%d\t%s\t%s\n", tasks[i].id, tasks[i].title, tasks[i].category,
                tasks[i].priority, tasks[i].completed, tasks[i].due_date, recurrence_strings[tasks[i].recurrence]);
    }

    fclose(file);
}

void display_tasks(Task *tasks, int count, int selected) {
    clear();  // Clear the screen for updating

    if (count == 0) {
        mvprintw(2, 0, "No tasks to display. Press 'a' to add a new task.");
        mvprintw(LINES - 2, 0, "Press 'h' for help.");
        refresh();
        return;
    }

    // Add headers
    mvprintw(0, 0, "ID  Title               Category        Priority  Due Date    Recurrence  Status");
    mvhline(1, 0, '-', COLS);

    for (int i = 0; i < count; i++) {
        int line = i + 2;

        if (line >= LINES - 2) {
            break;  // Prevent writing beyond the screen
        }

        if (i == selected) {
            attron(A_REVERSE);
        }
        if (is_task_overdue(tasks[i])) {
            attron(COLOR_PAIR(1));  // Red for overdue tasks
        } else if (is_task_due_soon(tasks[i])) {
            attron(COLOR_PAIR(2));  // Yellow for due soon tasks
        }

        // Set color based on priority
        int priority_color = tasks[i].priority + 2;  // Map priority 1-5 to color pairs 3-7
        attron(COLOR_PAIR(priority_color));

        char status[10];
        snprintf(status, sizeof(status), "%s", tasks[i].completed ? "Done" : "Pending");

        mvprintw(line, 0, "%-3d %-18s %-15s %-9d %-11s %-11s %-8s", 
                 tasks[i].id, tasks[i].title,
                 tasks[i].category, tasks[i].priority, tasks[i].due_date, recurrence_strings[tasks[i].recurrence], status);

        attroff(COLOR_PAIR(priority_color));

        if (is_task_overdue(tasks[i])) {
            attroff(COLOR_PAIR(1));  // Turn off overdue color
        } else if (is_task_due_soon(tasks[i])) {
            attroff(COLOR_PAIR(2));  // Turn off due soon color
        }

        if (i == selected) {
            attroff(A_REVERSE);
        }
    }
    mvprintw(LINES - 2, 0, "Press 'h' for help.");
    refresh();
}

int parse_date(const char *date_str, struct tm *date) {
    memset(date, 0, sizeof(struct tm));
    char *result = strptime(date_str, "%Y-%m-%d", date);
    return result != NULL;
}

void format_date(char *buffer, size_t size, struct tm *date) {
    strftime(buffer, size, "%Y-%m-%d", date);
}

RecurrenceType parse_recurrence(const char *str) {
    for (int i = 0; i < sizeof(recurrence_strings)/sizeof(recurrence_strings[0]); i++) {
        if (strcmp(str, recurrence_strings[i]) == 0) {
            return (RecurrenceType)i;
        }
    }
    return -1;
}

void undo_last_action(Task **tasks, int *count, int *capacity) {
    if (action_count == 0) {
        mvprintw(LINES - 2, 0, "Nothing to undo. Press any key...");
        refresh();
        getch();
        return;
    }
    action_count--;
    Action last_action = action_stack[action_count];
    switch (last_action.type) {
        case ACTION_ADD:
            // Remove the last added task
            remove_task(tasks, count, last_action.index);
            break;
        case ACTION_DELETE:
            // Restore the deleted task
            ensure_capacity(tasks, capacity, *count + 1);
            for (int i = *count; i > last_action.index; i--) {
                (*tasks)[i] = (*tasks)[i - 1];
            }
            (*tasks)[last_action.index] = last_action.task;
            (*count)++;
            break;
        case ACTION_EDIT:
            // Restore the previous state of the task
            (*tasks)[last_action.index] = last_action.task;
            break;
        case ACTION_COMPLETE:
            // Toggle back the completion status
            (*tasks)[last_action.index] = last_action.task;
            break;
    }
    mvprintw(LINES - 2, 0, "Last action undone. Press any key...");
    refresh();
    getch();
    log_message("Undo last action.");
}

void show_help() {
    clear();
    mvprintw(0, 0, "Help Menu");
    mvhline(1, 0, '-', COLS);
    mvprintw(2, 0, "Navigation:");
    mvprintw(3, 2, "'j' - Move down");
    mvprintw(4, 2, "'k' - Move up");
    mvprintw(5, 0, "Actions:");
    mvprintw(6, 2, "'a' - Add a new task");
    mvprintw(7, 2, "'d' - Delete the selected task");
    mvprintw(8, 2, "'e' - Edit the selected task");
    mvprintw(9, 2, "'c' - Toggle completion status");
    mvprintw(10, 2, "'s' - Search for a task");
    mvprintw(11, 2, "'P' - Sort tasks by priority");
    mvprintw(12, 2, "'S' - Sort tasks by due date");
    mvprintw(13, 2, "'u' - Undo last action");
    mvprintw(14, 2, "'h' - Show this help menu");
    mvprintw(15, 2, "'q' - Quit the application");
    mvprintw(LINES - 2, 0, "Press any key to return.");
    refresh();
    getch();
}

void log_message(const char *message) {
    char log_path[512];
    snprintf(log_path, sizeof(log_path), "%s/%s", getenv("HOME"), LOG_FILE_PATH);
    FILE *log_file = fopen(log_path, "a");
    if (log_file != NULL) {
        time_t now = time(NULL);
        char time_str[26];
        ctime_r(&now, time_str);
        time_str[strlen(time_str) - 1] = '\0';  // Remove newline character
        fprintf(log_file, "%s: %s\n", time_str, message);
        fclose(log_file);
    }
}

void handle_error(const char *message) {
    mvprintw(LINES - 2, 0, "%s", message);
    refresh();
    log_message(message);
}

char *get_database_path() {
    static char file_path[512];  // To store the resolved path
    char *home = getenv("HOME");
    snprintf(file_path, sizeof(file_path), "%s/%s", home, LOCAL_FILE_PATH);

    // Ensure the directory exists, create it if not
    char dir_path[512];
    snprintf(dir_path, sizeof(dir_path), "%s/.local/share/todo", home);
    if (access(dir_path, F_OK) != 0) {
        mkdir(dir_path, 0755);  // Create the directory with appropriate permissions
    }

    // Check if the tasks.txt file exists, create if not
    if (access(file_path, F_OK) != 0) {
        int fd = open(file_path, O_CREAT | O_RDWR, 0644);  // Create file with read/write for owner, read for others
        if (fd != -1) {
            close(fd);  // Close the file after creating it
        }
    }

    // Ensure the log file directory exists
    char log_dir_path[512];
    snprintf(log_dir_path, sizeof(log_dir_path), "%s/.local/share/todo", home);
    if (access(log_dir_path, F_OK) != 0) {
        mkdir(log_dir_path, 0755);  // Create directory if it doesn't exist
    }

    return file_path;
}
