![](https://github.com/mgreenly/space-captain/blob/beef285e4db72535b4698883c6fbbe68606f551e/img/spacecaptain-01.png)

Space Captain
=============

What is this?  It's a learning project centered around building a online multi-player game that can best be described
as a blend of Eve Online and Dwarf Fortress with a sprinkling of Factorio presented via a text-based interface.

Be forewarned this is more about learning "C" and Linux programming than actually finishing a game.

You can follow along at https://www.youtube.com/channel/UC7JijJhmmKrXaGyAxSJNBsQ


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
