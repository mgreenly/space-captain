![](https://github.com/mgreenly/space-captain/blob/faa6752349538bfa05d595e004a8d658b0f5098d/data/spacecaptain-01.png)

Space Captain
=============

Space Captain is a client-server MMO project built in C as a technical exploration of Linux network programming and systems optimization. The project evolves from a foundational technical demo (v0.1.0) featuring CLI-based space combat with 5,000 concurrent connections, through platform-specific graphical clients (v0.2.0-0.4.0), to a persistent world with crew progression, economy, and manufacturing systems (v0.5.0+). This methodical approach prioritizes robust infrastructure and incremental feature development, ultimately targeting a game that blends elements of Eve Online, Dwarf Fortress, and Factorio.

You can follow along at https://www.youtube.com/@LinuxProgrammingandStuff

Installing
==========

```
git clone https://github.com/mgreenly/space-captain.git
cd space-captian
make clean; PREFIX=$HOME/.local/bin make install
```
