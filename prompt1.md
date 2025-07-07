Familiarize yourself with the unified build arhitecture of this project.

Lets write a detailed workplan to prompt2.md and change no other files

The plan should be to remove the use of the unified build pattern and replace it with a more traditional project structure where each make target includes the requried header files.  Use the "-MMD -MP" flags of the compiler to generate each make targets dependencies.  The Makefile should have no explicit references to any *.c or *.h files except those for the targets.
