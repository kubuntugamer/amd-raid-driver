# 🏎️ SPEC 02: Asynchronous I/O & Memory Routing Pipeline

## 1. Asynchronous Control Path via io_uring
FabricZC completely bypasses the Linux kernel s traditional synchronous file read/write operational paths, which force thread sleeping states and heavy context-switch overhead.
* **Kernel Polling Engine:** All I/O transactions are driven via polled io_uring ring submission and completion queues using the IORING_SETUP_SQPOLL configuration flag.
* **Execution Boundary:** A dedicated kernel thread constantly polls the ring queues, submitting block data transfers directly to the NVMe driver layers without requiring individual system calls from running applications.

## 2. Zero-Copy P2PDMA VRAM Shortcuts
The data routing pipeline integrates the Linux kernel s native Peer-to-Peer DMA (pci_p2pdma) library subroutines to bridge storage and graphics hardware directly.
* **Page Mapping Intercept:** When an I/O payload page structure is evaluated, the driver evaluates the memory address parameters.
* **Host RAM Bypass:** If the destination page belongs to a registered PCIe GPU memory map allocation, the driver commands the NVMe hardware controllers to stream the data blocks directly over the PCIe bus traces straight into graphics VRAM, bypassing host system memory caches entirely.