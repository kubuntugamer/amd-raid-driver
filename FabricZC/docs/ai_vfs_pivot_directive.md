We are resuming development on the FabricZC custom filesystem and storage container project now that my physical bare-metal hardware (Ryzen 7 9700X + X870E Tomahawk platform) is active. 

The previous code iterations hit major development snags because the architecture was trying to force standard, synchronous VFS tracking structures and lock mechanisms to directly govern our asynchronous I/O paths. This created massive block-boundary translation errors, pointer arithmetic bloat, and Unreclaimable Slab allocation memory spikes.

We need to pivot the architecture. Pivot to a design where the VFS layer acts strictly as a thin, lightweight translation wrapper for terminal namespaces (e.g., standard mount, directory lookup, and metadata listing visibility). 

For the actual data path, strip out all monolithic VFS page-cache allocations and generic file-operation serialization loops. Enforce direct-to-flash block boundaries matching our hardware geometry natively. Wire the file read/write iteration entry points to immediately drop upper-level system locks, package requests into clean, aligned block chunks, and hand them off directly to our kamd_async_worker mirror completion queue and rc_amd_map_nested_raid10 routing routines.

Provide the updated code modules following a highly modular, decoupled design structure. Ensure there are no hardcoded targets or layout sizes, and keep the logic lean to fit our 128 MB fixed RAM profile.
