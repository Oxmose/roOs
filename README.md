
<p  align="center">

<img  src="https://github.com/Oxmose/roOs/Doc/logo/roos_logo.png"  width="300">

</p>

## roOs - Kernel
* roOs is a kernel created for training and educational purposes. It is the result of multiple hours of work to better understand the underlying concepts related to operating systems.
* The kernel is designed around real-time operating systems concepts and will not be intended to perform as a general-purpose operating system.

## Build status

| Status | Main | Dev |
| --- | --- | --- |
| CI | [![roOs CI Plan](https://github.com/Oxmose/roOs/actions/workflows/github-action-qemu.yml/badge.svg?branch=main)](https://github.com/Oxmose/roOs/actions/workflows/github-action-qemu.yml) | [![roOs CI Plan](https://github.com/Oxmose/roOs/actions/workflows/github-action-qemu.yml/badge.svg?branch=dev)](https://github.com/Oxmose/roOs/actions/workflows/github-action-qemu.yml) |
| Codacy | [![Codacy Badge](https://app.codacy.com/project/badge/Grade/d02a03d7f40a4a0e8b6821c6be95aa31)](https://app.codacy.com/gh/Oxmose/roOs/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade) | [![Codacy Badge](https://app.codacy.com/project/badge/Grade/d02a03d7f40a4a0e8b6821c6be95aa31)](https://app.codacy.com/gh/Oxmose/roOs/dashboard?utm_source=gh&utm_medium=referral&utm_content=&utm_campaign=Badge_grade) |

## Supported Architectures

| i386                                           | x64                          |
|------------------------------------------------|------------------------------|
| **Status**<br>Work in Progress                 | **Status**<br>Work in Progress |
| **Supported Processing**<br>Multicore, Singlecore | **Supported Processing**<br>Multicore, Singlecore |
| **Supported Drivers**<br>PIC<br>IO-APIC<br>Local APIC<br>ACPI<br>Serial Ports<br>RTC<br>PIT<br>Local APIC Timers<br>TSC<br>VGA Text 80x25<br>Generic PS/2 Keyboard | **Supported Drivers**<br>PIC<br>IO-APIC<br>Local APIC<br>ACPI<br>Serial Ports<br>RTC<br>PIT<br>Local APIC Timers<br>TSC<br>VGA Text 80x25<br>Generic PS/2 Keyboard |
| | |

## Supported General Features
### File Device Tree
* FDT is used to setup the system and discover drivers
* FDT handles used for drivers inter-operability
* Configuration (stdin, stdout, interrupts, etc.) in FDT

### Driver Management
* Easy-to-use driver system
* Driver attach mechanism with FDT auto-discovery

### Memory Management
* Higher-half kernel
* Virtual and Physical memory allocator
* Kernel Heap allocator

### Interrupt and Exception Management
* Custom Interrupts
* Custom Exceptions

### Synchronization
* Futex based synchronization
* Mutex: Non recursive/Recursive - Priority inheritance capable - FIFO and Priority-based queuing disciplines.
* Semaphore: FIFO and Priority-based queuing disciplines - Counting and Binary semaphores.
* Spinlocks
* Kernel critical section by disabling interrupts

### Communication
* Signals, not really POSIX signals but still signals
* Inter-processor interrupts

### Scheduler
* Priority based scheduler (Round Robin for all the threads having the same priority)
* Kernel threads
* Updateable priorities
* Sleep capable

### Kernel Tracing
* Manual trace generation
* Trace-Compass compatible trace stream generation

### Libraries
* Embedded LibC
* Structures Libraries (Vector, HashTable, Queues, etc.)

## Roadmap
You can see the planned tasks / features in the project's kanban on GitHub: https://github.com/users/Oxmose/projects/2

## Testing
roOs has a test suite allowing extensible testing. Using internal memory test buffers, the testing framework allows validating the system's correct behavior.
Tests can be added as new features are developed.

## Build and Run
To build roOs, choose the architecture you want and execute.
Architecture list to use in the TARGET flag:

* x86_i386
* x86_64

### Compilation
```
make target=[TARGET] TESTS=[TRUE/FALSE] DEBUG=[TRUE/FALSE] TRACE=[TRUE/FALSE]
```
### Execution
```
make target=[TARGET] run
```
### Tests, Trace and Debug

* TESTS flag set to TRUE to enable internal testing
* DEBUG flag set to TRUE to enable debuging support (-O0 -g3)
* TRACE flag set to TRUE to enable kernel tracing