# Xbox Object Directory List
Xbox Object Directory List using the open source nxdk

This is a tool for dump opened object directory list into object_directory_list.log file to research list of object directory exists from all types of original xbox console.

What the output result will look like on retail console is:
```
  \Device - Type = Dire
         \CdRom0 - Type = Devi
                \AUDIO_TS
                \VIDEO_TS
                ...
         \Harddisk0 - Type = Disk
                   \Partition0 - Type = Devi
                   \Partition1 - Type = Devi
                              \TDATA
                              \UDATA
                              ...
                   \Partition2 - Type = Devi
                              \xboxdash.xbe
                              ...
                   \Partition3 - Type = Devi
                              ...
                   \Partition4 - Type = Devi
                              ...
                   \Partition5 - Type = Devi
                              ...
  \Win32NamedObjects - Type = Dire
                    \TestSemaphoreObjectName - Type = Sema
                    \TestMutexObjectName - Type = Muta
  \?? - Type = Dire
     \D: - Type = Symb
      full path = \Device\Harddisk0\Partition1\xbox_object_directory_list
     \CdRom0: - Type = Symb
      full path = \Device\CdRom0
```

# How to Build:
All you need is nxdk. You can get it from https://github.com/XboxDev/nxdk link.

Then follow the setup guide from https://github.com/XboxDev/nxdk/wiki/Getting-Started link.

If you do not wish to download nxdk, you can freely use dev container built-in support locally or with GitHub's Codespaces.

# Binaries:
You can download pre-built binaries from here: https://github.com/RadWolfie/xbox_object_directory_list/releases

# GitHub CI:
Current build status can be seen here: https://github.com/RadWolfie/xbox_object_directory_list/actions/workflows/ci.yml

# Contributing
We welcome any form of contributions long as you agree with their respective source file's licensing.
