Small Linux crypto project with the niche goal of having encrypted .txt files on a removable drive without an encrypted partition. 

When the monitor param is passed, dbus will be monitored for signals from udisks of drives being mounted. These drives are then checked for a manifest.list file on the root of the partition. This file contains relative paths to files, one per line. All files listed are moved to a location under /tmp/workspace_*timestamp*, with encrypted files being decrypted. In this temp dir, files can be added or modified, and can be committed back to the drive by replacing the 0 in COMMIT_FILES with a 1.


Planned: 
- Implement reading from file streams
- File & title padding
