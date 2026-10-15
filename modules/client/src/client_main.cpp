#line 1 "C:/code-frontiers-gm/game/client_main.cpp"
// client_main -- CLIENT 0x00CC6EB0, the module's entry point and its one export.
// Byte-exact (phase 62, 2026-10-15): the first CLIENT function matched.
//
// The whole game is one object, VIClient (game.h), built on this function's
// stack.  Init takes the command line; the loop runs one frame at a time until
// the quit flag is set; the exit code is read out of the object before its
// destructor runs.
//
// "RELEASE executable size" measures the image: `_end` less the EE's load
// address, 0x100000, in megabytes -- a mul.s by 2^-20, then fptodp for the
// varargs promotion.
//
// Two source facts, each measured:
// - `end` is a NAMED local.  Written inline, `(int)_end - 0x100000` folds into one
//   address constant (a %hi/%lo of `_end-0x100000`); the target adds the
//   0xFFF00000 at run time.  27 -> 15 wrong.
// - `cmd` is declared AFTER the pf and BEFORE the client.  The empty string is
//   then in s0 across the constructor, which is where the target loads it (and
//   it settles which of pf's argument moves fills the delay slot).  Declared
//   before the pf it costs an extra instruction; passed as a literal, the
//   address is built after the constructor.  15 -> 0.
// The target's "" sits at the very front of this file's .rodata, ahead of the
// unreferenced table-name strings its headers' static initialisers emit.
#include "game.h"

extern char _end[];
// (blank lines below pad the assert to its original line, 52)
















int client_main(char *a, char *b)
{
    int end = (int)_end;
    pf("RELEASE executable size: %.2fMB\n", (float)(end - 0x100000) / (1024 * 1024));
    char *cmd = "";

    VIClient client;
    int r = client.Init(cmd, a, 0, b, 0, 0, 0, 1);
    VIASSERTF(r == 0, "Client.Init failed %i", r);

    while (client.IsRunning())
        client.Process();

    client.Shutdown();
    return client.m_exitCode;
}
