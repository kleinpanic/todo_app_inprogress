#ifndef TODO_H
#define TODO_H

#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <ncurses.h>
#include <stdbool.h>

// Define necessary constants
#define MAX_TITLE_LEN 256
#define MAX_CATEGORY_LEN 50
#define MAX_DATE_LEN 12  // Adjusted to fit YYYY-MM-DD format with null terminator
#define MAX_RECURRENCE_LEN 10
#define LOCAL_FILE_PATH ".local/share/todo/tasks.txt"
#define LOG_FILE_PATH ".local/share/todo/todo_app.log"
#define NO_DUE_DATE "N/A"  // Custom marker for no due date

// Define maximum number of actions for undo functionality
#define MAX_ACTIONS 100

// Define action types for undo functionality
typedef enum {
    ACTION_ADD,
    ACTION_DELETE,
    ACTION_EDIT,
    ACTION_COMPLETE
} ActionType;

// Define recurrence types
typedef enum {
    RECURRENCE_NONE,
    RECURRENCE_DAILY,
    RECURRENCE_WEEKLY,
    RECURRENCE_BIWEEKLY,
    RECURRENCE_MONTHLY,
    RECURRENCE_YEARLY
} RecurrenceType;

extern const char *recurrence_strings[];

// Task structure definition
typedef struct {
    int id;
    char title[MAX_TITLE_LEN];
    char category[MAX_CATEGORY_LEN];
    char due_date[MAX_DATE_LEN];
    RecurrenceType recurrence;
    int priority;
    int completed;
} Task;

// Action structure for undo functionality
typedef struct {
    ActionType type;
    Task task;
    int index;  // Position of the task in the list
} Action;

// Function prototypes
void add_task(Task **tasks, int *count, int *capacity, const char *title, const char *category, const char *due_date, RecurrenceType recurrence, int priority);
void remove_task(Task **tasks, int *count, int index);
void edit_task(Task *task);
void load_tasks(Task **tasks, int *count, int *capacity);
void save_tasks(Task *tasks, int count);
void display_tasks(Task *tasks, int count, int selected);
void search_task(Task *tasks, int count, int *selected_task);
void sort_tasks(Task *tasks, int count, char sort_type, bool ascending);
void get_input(char *buffer, int size, const char *prompt);
void get_input_and_clear(char *buffer, int size, const char *prompt);
void init_ncurses();
void cleanup_ncurses();
void toggle_task_completion(Task *task);
void update_task_recurrence(Task *task);
int is_task_overdue(Task task);
int is_task_due_soon(Task task);
int parse_date(const char *date_str, struct tm *date);
void format_date(char *buffer, size_t size, struct tm *date);
RecurrenceType parse_recurrence(const char *str);
void show_help();
void ensure_capacity(Task **tasks, int *capacity, int needed);
void undo_last_action(Task **tasks, int *count, int *capacity);
void log_message(const char *message);
void trigger_save_tasks(Task *tasks, int count);
void *save_tasks_async(void *arg);
void handle_error(const char *message);
void update_task_ids(Task *tasks, int count);
char *get_database_path();

#endif
