<p align="center">
	<img src="https://github.com/Oxmose/UTK-Reboot/raw/main/Doc/logo/utk_logo.png" width="300">
</p>

## UTK - Utility Kernel

* UTK is a kernel created for training and educational purposes. It is designed to execute in kernel mode only.
* UTK has a configuration file allowing the kernel to be customized depending on the system it will run on.

----------

## UTK Build status


| Status | Main | Dev |
| --- | --- | --- |
| CI | [![UTK CI Plan](https://github.com/Oxmose/nUTK/actions/workflows/github-action-qemu.yml/badge.svg?branch=main)](https://github.com/Oxmose/nUTK/actions/workflows/github-action-qemu.yml) | [![UTK CI Plan](https://github.com/Oxmose/nUTK/actions/workflows/github-action-qemu.yml/badge.svg?branch=dev)](https://github.com/Oxmose/nUTK/actions/workflows/github-action-qemu.yml) |
| Codacy | [![Codacy Badge](https://app.codacy.com/project/badge/Grade/d02a03d7f40a4a0e8b6821c6be95aa31)](https://app.codacy.com/gh/Oxmose/nUTK/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade) | [![Codacy Badge](https://app.codacy.com/project/badge/Grade/d02a03d7f40a4a0e8b6821c6be95aa31)](https://app.codacy.com/gh/Oxmose/nUTK/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade) |


----------

## Supported Architectures
| i386 | x86_64 | ARM |
| --- | --- | --- |
|  WIP |   WIP | NO |

### Memory Management

* Higher-half kernel
* Virtual/physical memory allocator
* Process isolation

### Synchronization

* Futex based synchronization
* Mutex: Non recursive/Recursive - Priority inheritance capable. Futex based
* Semaphore: FIFO based, priority of the locking thread is not relevant to select the next thread to unlock. Futex based
* Spinlocks
* Kernel critical section by disabling interrupts

### Scheduler

* Priority based scheduler (threads can change their priority at execution time)
* Round Robin for all the threads having the same priority

### Process / Thread management

* Fork / WaitPID
* Kernel threads support
* System calls

### bsp Support: i386/x86_64

* PIC and IO-APIC support (PIC supported but IO APIC required)
* Local APIC support with timers
* PIT support (can be used as an auxiliary timer source)
* RTC support
* Basic ACPI support (simple parsing used to enable multicore features)
* Serial output support
* Interrupt API (handlers can be set by the user)
* 80x25 16colors VGA support
* Time management API.

----------

## Roadmap
You can see the planned tasks / features in the project's kanban on GitHub.

----------
## Build and run
To build UTK, choose the architecture you want and execute.

Architecture list to use in the TARGET flag:
* x86_i386
* x86_64

### Compilation
make target=[TARGET] TESTS=[TRUE/FALSE] DEBUG=[TRUE/FALSE] TRACE=[TRUE/FALSE]

### Execution
make target=[TARGET] run

### Tests and Debug
* TESTS flag set to TRUE to enable internal testing
* DEBUG flag set to TRUE to enable debuging support (-O0 -g3)
* TRACE flag set to TRUE to enable kernel tracing