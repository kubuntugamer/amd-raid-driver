# 🔀 SPEC 04: Online Capacity Expansion (OCE) Sequence Lifecycle

## 1. Capacity Scale Bounds
The storage container engine implements code gates that restrict active topology scaling boundaries cleanly between **1 solitary boot disk device up to a maximum ceiling of 8 concurrent NVMe block device nodes** under a 1 MiB chunk stripe.

## 2. The 4-Phase Online Capacity Expansion Sequence
When a user-space instruction signals the container to append raw physical block devices onto a live, running array footprint, the kernel subsystem coordinates layout shifts in a strict 4-phase loop:

### Phase 1: Allocation Ring Freezing
* The driver temporarily pauses active userspace io_uring ring submission submissions to freeze storage transactions safely.

### Phase 2: Boundary Clearing
* The system triggers raw, low-level dd blocks to zero out the 16 KiB front-offset anchor zones and trailing sectors of the incoming new drive nodes, clearing foreign residues completely.

### Phase 3: Metadata Stamping
* The engine increments the total_active_disks counter variable inside the 48-byte header, recomputes the Fletcher-64 checksum, and flashes the new layout configuration parameters across all member drives concurrently.

### Phase 4: Asynchronous Stripe Activation
* Queues are unfrozen. Historical boot blocks resting on your original single disk remain completely static and unshifted on their physical tracks on the fly, eliminating any active data loss risks. 
* All new write transactions alternate across the expanded striped layout immediately. When the system hits complete idle states, a background rebalancer thread smoothly copies historical tracks and sweeps them into a perfectly interleaved horizontal stripe.