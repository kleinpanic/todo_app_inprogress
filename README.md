# Todo CLI Application

A command-line based todo list application built with C and ncurses. This application allows you to manage tasks directly from your terminal with an intuitive interface.

## Table of Contents

- [Features](#features)
- [Installation](#installation)
  - [Dependencies](#dependencies)
  - [Building the Application](#building-the-application)
  - [Installing the Application](#installing-the-application)
- [Usage](#usage)
  - [Key Bindings](#key-bindings)
  - [Task Fields](#task-fields)
- [Data Storage](#data-storage)
- [Additional Information](#additional-information)
- [Uninstallation](#uninstallation)
- [Contributing](#contributing)

## Features

- **Add Tasks**: Create new tasks with a title, category, due date, priority, and recurrence.
- **Edit Tasks**: Modify existing tasks.
- **Delete Tasks**: Remove tasks from your list.
- **Complete Tasks**: Mark tasks as completed or pending.
- **Search Tasks**: Find tasks by their title.
- **Sort Tasks**: Sort tasks by priority or due date.
- **Undo Actions**: Undo the last action performed.
- **Recurring Tasks**: Set tasks to recur at specified intervals.
- **Color-coded Tasks**: Visual cues for overdue or due soon tasks.
- **Persistent Storage**: Tasks are saved between sessions.

## Installation

### Dependencies

Ensure that you have the following dependencies installed on your system:

- **gcc**: GNU Compiler Collection for compiling the C code.
- **make**: Utility for directing compilation.
- **ncurses**: Library for creating text-based user interfaces.
- **pthread**: POSIX threads library for multi-threading support.

**On Debian/Ubuntu:**

```bash
sudo apt-get update
sudo apt-get install build-essential libncurses5-dev libpthread-stubs0-dev
```

**On Fedora:**

```bash
sudo dnf install gcc make ncurses-devel glibc-devel
```

**On Arch Linux:**

```bash
sudo pacman -S base-devel ncurses
```

### Building the Application

1. **Clone the Repository**

   ```bash
   git clone https://github.com/kleinpanic/todo_task_manager.git
   git clone git@github.com:kleinpanic/todo_task_manager.git
   cd todo-cli
   ```

2. **Build the Application**

   ```bash
   make
   ```

   This will compile the source code and create the executable in the `build/` directory.

### Installing the Application

Optionally, you can install the application system-wide:

```bash
sudo make install
```

This will copy the executable to `/usr/local/bin/todo` and make it accessible from anywhere in your terminal.

## Usage

Run the application from the `build/` directory:

```bash
./todo
```

Or, if installed system-wide:

```bash
todo
```

### Key Bindings

- **Navigation**
  - `j`: Move down in the task list.
  - `k`: Move up in the task list.
- **Actions**
  - `a`: Add a new task.
  - `d`: Delete the selected task.
  - `e`: Edit the selected task.
  - `c`: Toggle completion status of the selected task.
  - `s`: Search for a task by title.
  - `P`: Sort tasks by priority (toggle ascending/descending).
  - `S`: Sort tasks by due date (toggle ascending/descending).
  - `u`: Undo the last action.
  - `h`: Show the help menu.
  - `q`: Quit the application.

### Task Fields

When adding or editing a task, you'll be prompted for the following information:

- **Title**: A brief description of the task (cannot be empty).
- **Category**: The category or project the task belongs to (cannot be empty).
- **Due Date**: The due date in `YYYY-MM-DD` format or `N/A` if not applicable.
- **Recurrence**: How often the task recurs (`none`, `daily`, `weekly`, `biweekly`, `monthly`, `yearly`).
- **Priority**: An integer between 1 (highest priority) and 5 (lowest priority).

## Data Storage

Your tasks are stored in a plain text file located at:

```
~/.local/share/todo/tasks.txt
```

The application ensures that this directory and file are created if they do not exist.

**Log File**

Any logs or error messages are recorded in:

```
~/.local/share/todo/todo_app.log
```

## Additional Information

- **Customizing Colors**: You can modify the color schemes used for task highlighting by editing the `init_ncurses()` function in `task.c`. Adjust the `init_pair` functions to change colors as desired.

- **Extending Functionality**: Feel free to fork the repository and add new features or improve existing ones. Contributions are welcome!

## Uninstallation

To remove the application and its associated files:

1. **Remove the Executable**

   If installed system-wide:

   ```bash
   sudo make uninstall
   ```

2. **Delete Configuration and Data Files**

   ```bash
   rm -rf ~/.local/share/todo
   ```

   **Warning**: This will permanently delete all your saved tasks.

## Contributing

Contributions are welcome! If you have ideas for new features or improvements, feel free to fork the repository and submit a pull request. Let's make this application even better together.

---
