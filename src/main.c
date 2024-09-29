#include <ncurses.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>
#include "todo.h"

#define ACTIONS_BEFORE_AUTOSAVE 5

Task *tasks = NULL;          // Dynamic array of tasks
int task_count = 0;          // Number of tasks
int task_capacity = 0;       // Capacity of the tasks array
int selected_task = 0;       // Index of the currently selected task

// Booleans to track sort order
bool priority_ascending = true;  // Start with highest to lowest priority
bool date_ascending = true;      // Start with closest to latest due date

// Action stack for undo functionality
Action action_stack[MAX_ACTIONS];
int action_count = 0;

// Mutex for thread safety
pthread_mutex_t task_mutex = PTHREAD_MUTEX_INITIALIZER;

// Counter for autosave
int action_counter = 0;

// Function to clear input prompt after getting the input
void get_input_and_clear(char *buffer, int size, const char *prompt) {
    mvprintw(LINES - 2, 0, "%s", prompt);
    clrtoeol();  // Clears the prompt area before input to avoid overlay
    echo();
    getnstr(buffer, size - 1);  // Get user input
    noecho();
    buffer[size - 1] = '\0';  // Null-terminate the string
    clear();  // Clear the entire screen after each input
    refresh();
}

// Function to delete a task interactively with added safety checks
void delete_task_interactive() {
    mvprintw(LINES - 2, 0, "Are you sure you want to delete this task? (y/n): ");
    clrtoeol();
    refresh();
    int ch = getch();
    
    if (ch == 'y' || ch == 'Y') {
        pthread_mutex_lock(&task_mutex);

        // Check if selected_task index is within valid bounds
        if (selected_task < 0 || selected_task >= task_count) {
            pthread_mutex_unlock(&task_mutex);
            mvprintw(LINES - 2, 0, "Error: Invalid task selected. Press any key...");
            refresh();
            getch();
            return;
        }

        // Push the action onto the stack before deletion
        if (action_count < MAX_ACTIONS) {
            action_stack[action_count].type = ACTION_DELETE;
            action_stack[action_count].task = tasks[selected_task];
            action_stack[action_count].index = selected_task;
            action_count++;
        }

        remove_task(&tasks, &task_count, selected_task);
        update_task_ids(tasks, task_count);

        pthread_mutex_unlock(&task_mutex);

        if (selected_task >= task_count && task_count > 0) {
            selected_task = task_count - 1;
        }

        log_message("Task deleted.");
        action_counter++;

        if (action_counter >= ACTIONS_BEFORE_AUTOSAVE) {
            trigger_save_tasks(tasks, task_count);
            action_counter = 0;
        }
    }
    clear();
}

int main() {
    init_ncurses();
    
    load_tasks(&tasks, &task_count, &task_capacity);

    // Ensure that tasks is initialized
    if (tasks == NULL) {
        endwin();  // Cleanup ncurses before exiting
        fprintf(stderr, "Error: Failed to initialize tasks.\n");
        exit(1);
    }

    display_tasks(tasks, task_count, selected_task);  // Initial display
    int ch;
    
    while ((ch = getch()) != 'q') {
        pthread_mutex_lock(&task_mutex);

        switch (ch) {
            case 'j':
                if (selected_task < task_count - 1) selected_task++;
                break;
            case 'k':
                if (selected_task > 0) selected_task--;
                break;
            case 'a':
                add_task(&tasks, &task_count, &task_capacity, "", "", "", RECURRENCE_NONE, 0);
                update_task_ids(tasks, task_count);
                action_counter++;
                if (action_counter >= ACTIONS_BEFORE_AUTOSAVE) {
                    trigger_save_tasks(tasks, task_count);
                    action_counter = 0;
                }
                break;
            case 'd':
                if (task_count > 0) {
                    delete_task_interactive();
                } else {
                    mvprintw(LINES - 2, 0, "No tasks to delete. Press any key...");
                    refresh();
                    getch();
                }
                break;
            case 'c':
                if (selected_task >= 0 && selected_task < task_count) {
                    toggle_task_completion(&tasks[selected_task]);
                    action_counter++;
                    if (action_counter >= ACTIONS_BEFORE_AUTOSAVE) {
                        trigger_save_tasks(tasks, task_count);
                        action_counter = 0;
                    }
                }
                break;
            case 'e':
                if (selected_task >= 0 && selected_task < task_count) {
                    edit_task(&tasks[selected_task]);
                    action_counter++;
                    if (action_counter >= ACTIONS_BEFORE_AUTOSAVE) {
                        trigger_save_tasks(tasks, task_count);
                        action_counter = 0;
                    }
                }
                break;
            case 's':  // Search functionality
                search_task(tasks, task_count, &selected_task);
                break;
            case 'P':  // Toggle priority sorting
                sort_tasks(tasks, task_count, 'p', priority_ascending);
                update_task_ids(tasks, task_count);
                priority_ascending = !priority_ascending;  // Toggle the boolean
                selected_task = 0;  // Reset selection to the first task after sorting
                break;
            case 'S':  // Toggle due date sorting
                sort_tasks(tasks, task_count, 'd', date_ascending);
                update_task_ids(tasks, task_count);
                date_ascending = !date_ascending;  // Toggle the boolean
                selected_task = 0;  // Reset selection to the first task after sorting
                break;
            case 'u':
                undo_last_action(&tasks, &task_count, &task_capacity);
                update_task_ids(tasks, task_count);
                break;
            case 'h':
                show_help();
                break;
            default:
                mvprintw(LINES - 2, 0, "Unknown command. Press 'h' for help.");
                refresh();
                break;
        }

        pthread_mutex_unlock(&task_mutex);
        
        // Ensure that selected_task is always within bounds
        if (selected_task < 0) selected_task = 0;
        if (selected_task >= task_count && task_count > 0) selected_task = task_count - 1;

        display_tasks(tasks, task_count, selected_task);
    }

    // Save tasks and clean up before exiting
    trigger_save_tasks(tasks, task_count);
    cleanup_ncurses();
    free(tasks);  // Free dynamically allocated tasks array
    return 0;
}
