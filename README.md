# FAT File System

This project was developed in **C** for the **Operating Systems** course at the Federal University of São João del-Rei (UFSJ), during the **first semester of 2025**.

The goal of the assignment was to implement a simulator of a simple file system based on a **16-bit File Allocation Table (FAT)**, together with a custom command-line shell used to interact with it.

The entire virtual file system is persisted in a single file named `fat.part`. The simulated partition has a fixed size of **4 MB**, divided into **4096 clusters of 1024 bytes each**.

A key requirement of the assignment was that file-system data could not simply be loaded entirely into memory and manipulated there. Directory entries and file contents had to be accessed through **cluster-sized disk operations**, while the FAT itself could be kept in memory and written back to the virtual partition.

## File System Layout

The virtual partition follows this structure:

```text
+-----------------------+
| Boot Block            |  1 cluster
+-----------------------+
| FAT                   |  8 clusters
+-----------------------+
| Root Directory        |  1 cluster
+-----------------------+
|                       |
| Data Clusters         |  4086 clusters
|                       |
+-----------------------+
```

The partition uses:

* **512-byte sectors**
* **1024-byte clusters**
* **4096 total clusters**
* **4096 16-bit FAT entries**
* **32-byte directory entries**
* **32 entries per directory**

The first cluster is reserved as the boot block, followed by eight clusters containing the FAT. The root directory occupies the next cluster, while the remaining clusters are available for files and subdirectories.

### FAT Entries

Each 16-bit FAT entry represents the state of a cluster:

| Value               | Meaning                                     |
| ------------------- | ------------------------------------------- |
| `0x0000`            | Free cluster                                |
| `0x0001` – `0xFFFC` | Pointer to the next cluster in a file chain |
| `0xFFFD`            | Boot block                                  |
| `0xFFFE`            | Reserved FAT cluster                        |
| `0xFFFF`            | End of file                                 |

### Directory Entries

Each directory entry occupies **32 bytes** and stores:

* file or directory name;
* type attribute;
* reserved bytes;
* first allocated cluster;
* file size.

Directories themselves occupy one cluster and therefore support up to **32 entries**.

## Implementation

The simulator stores the FAT in memory after loading the virtual partition, while file data and directory structures are accessed directly from `fat.part`.

Paths are resolved starting from the root directory and traversing directory entries until the requested file or directory is found.

Files larger than one cluster are represented through **FAT chains**, where each cluster points to the next allocated cluster until an `EOF` entry is reached.

The implementation handles:

* allocation and release of FAT clusters;
* hierarchical path traversal;
* file and directory creation;
* duplicate-name validation;
* fixed-size directory capacity;
* file overwriting;
* file appending;
* multi-cluster files;
* removal of allocated cluster chains;
* prevention of non-empty directory deletion;
* persistent updates to the virtual partition.

When appending data to an existing file, the implementation first uses any remaining space in the file's final cluster before allocating additional clusters when necessary.

## Shell Commands

The project includes a custom interactive shell for manipulating the virtual file system.

### `init`

Creates and initializes a new `fat.part` virtual partition.

```text
> init
Sistema de arquivos FAT inicializado com sucesso.
```

The operation initializes:

* the boot block;
* the FAT;
* the root directory;
* the remaining empty data clusters.

---

### `load`

Loads the FAT from an existing `fat.part` file.

```text
> load
Sistema de arquivos FAT carregado com sucesso.
```

If the partition does not exist:

```text
Erro! 'fat.part' não encontrado!
Execute 'init' primeiro.
```

---

### `ls`

Lists the contents of a directory.

```text
> ls /path/to/directory
```

Running `ls` without a path lists the root directory:

```text
> ls
```

Directory output follows this format:

```text
Attr| Tamanho | Nome
----|---------|----
 D   | 0       | docs
 A   | 25      | file.txt
```

`D` represents a directory and `A` represents a regular file.

If the supplied path points directly to a file, its information is displayed instead.

---

### `mkdir`

Creates a directory.

```text
> mkdir /docs
```

Nested paths are supported:

```text
> mkdir /docs/projects
```

The command verifies whether:

* the parent path exists;
* the parent is a directory;
* another entry with the same name already exists;
* the parent directory still has available entries;
* a free cluster is available.

Each new directory receives its own data cluster.

---

### `create`

Creates a new empty file.

```text
> create /docs/file.txt
```

A newly created file initially has:

```text
size = 0
first_block = 0
```

The command rejects duplicate names and file names longer than **17 characters**.

---

### `write`

Writes text to a file, replacing its existing contents.

```text
> write /docs/file.txt Hello
```

If the file already contains data, its existing FAT chain is released before the new content is allocated.

The number of clusters required is calculated according to the size of the new content, allowing files to span multiple clusters.

Example success message:

```text
Texto escrito no arquivo '/docs/file.txt' com sucesso.
```

The command also checks for conditions such as:

* nonexistent files;
* attempts to write to directories;
* insufficient free clusters.

---

### `append`

Adds text to the end of an existing file.

```text
> append /docs/file.txt World
```

The implementation first checks whether the last allocated cluster still contains unused space. That space is filled before additional clusters are allocated.

This avoids allocating a new cluster when the existing final cluster still has sufficient capacity.

---

### `read`

Reads and displays a file's contents.

```text
> read /docs/file.txt
```

The command follows the FAT chain from the file's first cluster until `EOF`, while respecting the stored file size so that only valid bytes are displayed.

Directories cannot be read as regular files.

---

### `unlink`

Removes a file or an empty directory.

```text
> unlink /docs/file.txt
```

For files, all clusters belonging to the FAT chain are marked as free again.

Directories can only be removed when they contain no entries.

Attempting to delete a non-empty directory results in an error.

---

### `exit`

Terminates the interactive shell.

```text
> exit
Saindo...
```

## Example Session

A simple interaction with the file system can look like this:

```text
> init
Sistema de arquivos FAT inicializado com sucesso.

> load
Sistema de arquivos FAT carregado com sucesso.

> mkdir /docs

> create /docs/hello.txt

> write /docs/hello.txt Hello

> append /docs/hello.txt World

> read /docs/hello.txt
HelloWorld

> ls /docs
Attr| Tamanho | Nome
----|---------|----
 A   | 10      | hello.txt

> unlink /docs/hello.txt

> unlink /docs

> exit
Saindo...
```

## Build and Run

The project was developed for a **Linux environment**.

Clone the repository and compile it using:

```bash
make
```

Run the interactive shell with:

```bash
make run
```

A C compiler and `make` are required.

## Project Structure

```text
.
├── fat.h
├── fat_commands.c
├── fat_helpers.c
├── main.c
├── Makefile
└── README.md
```

### `fat.h`

Defines constants, FAT values, data structures and function prototypes.

### `fat_commands.c`

Implements the file-system shell operations such as initialization, loading, directory listing, file creation, writing, appending, reading and deletion.

### `fat_helpers.c`

Contains helper functions for FAT manipulation, free-cluster lookup, directory-entry searches and hierarchical path resolution.

### `main.c`

Implements the interactive command-line shell and dispatches commands to the corresponding file-system operations.

## Academic Constraints

Some limitations of this implementation were deliberately defined by the assignment specification:

* fixed **4 MB** virtual partition;
* **4096 clusters**;
* **1024 bytes per cluster**;
* fixed-size 16-bit FAT;
* directories limited to **32 entries**;
* file and directory structures manipulated through cluster-sized disk operations.

These constraints were intended to simplify the implementation while preserving the core concepts involved in FAT-based file systems.

## Academic Context

Developed as part of the **Operating Systems** course of the Computer Science undergraduate program at **UFSJ — Federal University of São João del-Rei**, during the first semester of 2025.

The assignment focused on understanding file-system organization, persistent storage, allocation tables, directories and low-level data management.



