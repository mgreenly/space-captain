![](https://github.com/mgreenly/space-captain/blob/faa6752349538bfa05d595e004a8d658b0f5098d/dat/spacecaptain-01.png)

Space Captain
=============

What is this?  It's toy MMO and learning exercise centered around building a game that can best be described as a blend
of Eve Online and Dwarf Fortress with a sprinkling of Factorio presented.

You can follow along at https://www.youtube.com/@LinuxProgrammingandStuff


Dependencies
============

  **Required**
  * gcc or clang
  * pkg-config

  **Optional**
  * valgrind
  * ctags

Installing
==========

```
git clone https://github.com/mgreenly/space-captain.git
cd space-captian
make clean; make release; PREFIX=$HOME/.local/bin make install
```

Testing
=======
  https://github.com/ThrowTheSwitch/Unity/blob/b9d897b5f38674248c86fb58342b87cb6006fe1f/docs/UnityAssertionsCheatSheetSuitableforPrintingandPossiblyFraming.pdf
