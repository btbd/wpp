# WPP

A proof-of-concept for intercepting drivers' DeviceControl via WPP.

## Explanation

Windows drivers like `disk.sys` and `mountmgr.sys` support WPP tracing for debugging purposes within their `DeviceControl` (and other functions). Since the drivers' pointer to the global WPP control and pointer to the WPP trace function both reside in `.data` sections, its WPP can be easily hijacked.

After changing the WPP trace function and enabling the driver's WPP via changing the flags, we must determine if our trace function was actually called by `DeviceControl` via a return address check.

Finally, we want to intercept the `DeviceControl` by grabbing the pointer to the IRP. This can either be done by walking the stack backwards or using a register that still has the IRP pointer. In this PoC, a stack walk is used for `disk.sys` and a register is used for `mountmgr.sys`.

Obviously, this method only works on drivers that actually have WPP tracing inside their `DeviceControl`.

## Note

This PoC was only tested on Windows 10 1507, 1803, 1809, and 1903.