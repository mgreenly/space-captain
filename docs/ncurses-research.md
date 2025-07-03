# NCurses Research

## Overview

NCurses (New Curses) is a programming library that provides an API for building text-based user interfaces in a terminal-independent manner. It's a free software emulation of the System V Release 4.0 curses library, originally developed in the early 1980s. NCurses allows developers to:

- Control cursor movement
- Create windows and panels
- Handle keyboard and mouse input
- Display text with attributes (colors, bold, underline, etc.)
- Create forms and menus
- Build full-screen terminal applications

The library abstracts away terminal-specific control sequences, allowing the same code to work across different terminal types (xterm, gnome-terminal, konsole, etc.).

## NCurses Terminology

### Core Concepts

**Window**: A rectangular area of the screen with its own coordinate system. The default window is `stdscr` (standard screen).

**WINDOW**: The C structure representing a window object.

**Cursor**: The position where the next character will be written.

**Coordinates**: Always specified as (y, x) where y is the row and x is the column. Origin (0,0) is the top-left corner.

**Refresh**: The process of updating the physical screen to match the internal screen buffer.

### Key Functions

**initscr()**: Initialize the ncurses library and create stdscr.

**endwin()**: Clean up and restore terminal to normal mode.

**refresh()** / **wrefresh()**: Update the physical screen.

**getch()** / **wgetch()**: Read a character from the keyboard.

**move()** / **wmove()**: Move the cursor to a position.

**printw()** / **wprintw()**: Print formatted text (like printf).

**attron()** / **attroff()**: Turn attributes on/off (bold, color, etc.).

### Window Types

**stdscr**: The default full-screen window.

**Sub-windows**: Windows that share memory with their parent.

**Derived windows (pads)**: Windows that can be larger than the physical screen.

**Panels**: Windows that can overlap and be stacked.

## Versions and Linux Distribution Availability

### Current Versions

- **NCurses 6.x**: Current stable branch (6.4 as of 2023)
- **NCurses 5.x**: Legacy branch, still widely used
- **NCursesw**: Wide-character (Unicode) support variant

### Distribution Availability

**Debian/Ubuntu**:
```bash
# Development libraries
sudo apt-get install libncurses5-dev libncursesw5-dev
# Runtime libraries (usually pre-installed)
sudo apt-get install libncurses5 libncursesw5
```

**RHEL/CentOS/Fedora**:
```bash
# Development libraries
sudo yum install ncurses-devel
# or on newer versions
sudo dnf install ncurses-devel
```

**Arch Linux**:
```bash
# Usually pre-installed, but if needed:
sudo pacman -S ncurses
```

**Alpine Linux**:
```bash
apk add ncurses-dev
```

### Version Compatibility

Most distributions ship with both NCurses 5 and 6 for compatibility. The wide-character variant (ncursesw) is recommended for modern applications that need Unicode support.

## Cross-Platform Considerations

### Linux
Native support, works out of the box. Use pkg-config for compilation:
```bash
gcc myapp.c -o myapp $(pkg-config --cflags --libs ncursesw)
```

### macOS
NCurses is included in macOS, but it's often an older version. For newer features:
```bash
# Install via Homebrew
brew install ncurses
# Link against Homebrew version
gcc myapp.c -o myapp -I/usr/local/opt/ncurses/include -L/usr/local/opt/ncurses/lib -lncurses
```

### Windows

**Option 1: Windows Subsystem for Linux (WSL)**
- Full Linux compatibility
- Best option for development
- Native ncurses works without modification

**Option 2: Cygwin**
- Provides POSIX compatibility layer
- NCurses available through Cygwin packages
- Good for porting Unix applications

**Option 3: MSYS2/MinGW**
- Lighter than Cygwin
- NCurses available through pacman
- Good for native Windows builds

**Option 4: PDCurses**
- Native Windows implementation of curses API
- Drop-in replacement for many ncurses applications
- Doesn't require POSIX layer
- Some API differences from ncurses

**Option 5: Windows Terminal + Native Console APIs**
- Use Windows Console API directly
- More work but native performance
- Consider using a compatibility layer

### Cross-Platform Build Example
```c
#ifdef _WIN32
    #include <pdcurses.h>
#else
    #include <ncurses.h>
#endif
```

## Tips, Tricks, and Gotchas

### Best Practices

1. **Always check return values**: Many ncurses functions return ERR on failure.

2. **Initialize colors properly**:
```c
if (has_colors()) {
    start_color();
    init_pair(1, COLOR_RED, COLOR_BLACK);
}
```

3. **Handle terminal resize**:
```c
signal(SIGWINCH, handle_resize);
// In handler: endwin(); refresh(); clear();
```

4. **Use nodelay() for non-blocking input**:
```c
nodelay(stdscr, TRUE);  // getch() won't block
```

5. **Clean up properly**:
```c
endwin();  // Always call before exit
```

### Common Gotchas

1. **Coordinate Order**: It's (y, x), not (x, y)!
```c
mvprintw(10, 20, "Text");  // Row 10, Column 20
```

2. **Refresh Required**: Changes don't appear until refresh():
```c
printw("Hello");
refresh();  // Without this, nothing shows
```

3. **Input Echo**: Disable echo for password input:
```c
noecho();  // Don't show typed characters
```

4. **Buffering**: Use cbreak() for immediate input:
```c
cbreak();  // Don't wait for Enter key
```

5. **Color Limitations**: Only 64 color pairs by default (can be extended).

6. **Terminal Capabilities**: Not all terminals support all features.

7. **Wide Characters**: Use ncursesw for Unicode:
```c
#include <ncursesw/ncurses.h>
setlocale(LC_ALL, "");  // Enable Unicode
```

### Performance Tips

1. **Use wnoutrefresh() + doupdate()** for multiple window updates:
```c
wnoutrefresh(win1);
wnoutrefresh(win2);
doupdate();  // Single screen update
```

2. **Minimize full refreshes**: Use touchwin() sparingly.

3. **Use scrolling regions** for log windows:
```c
scrollok(window, TRUE);
```

## Implementation Examples for Your Use Case

### Creating Multiple Boxes
```c
#include <ncurses.h>

int main() {
    initscr();
    cbreak();
    noecho();
    
    int height = LINES;
    int width = COLS;
    
    // Create windows with borders
    WINDOW *win1 = newwin(height/2, width/2, 0, 0);
    WINDOW *win2 = newwin(height/2, width/2, 0, width/2);
    WINDOW *win3 = newwin(height/2, width, height/2, 0);
    
    // Draw borders
    box(win1, 0, 0);
    box(win2, 0, 0);
    box(win3, 0, 0);
    
    // Add titles
    mvwprintw(win1, 0, 2, " Status ");
    mvwprintw(win2, 0, 2, " Values ");
    mvwprintw(win3, 0, 2, " Log ");
    
    // Refresh all windows
    wrefresh(win1);
    wrefresh(win2);
    wrefresh(win3);
    
    getch();
    endwin();
    return 0;
}
```

### Scrolling Text Window
```c
WINDOW *create_log_window(int height, int width, int y, int x) {
    WINDOW *win = newwin(height, width, y, x);
    box(win, 0, 0);
    
    // Create inner window for scrolling content
    WINDOW *inner = derwin(win, height-2, width-2, 1, 1);
    scrollok(inner, TRUE);  // Enable scrolling
    
    return inner;
}

void log_message(WINDOW *log_win, const char *msg) {
    wprintw(log_win, "%s\n", msg);
    wrefresh(log_win);
}
```

### REPL Window
```c
typedef struct {
    WINDOW *output;
    WINDOW *input;
    char buffer[256];
} ReplWindow;

ReplWindow* create_repl(int height, int width, int y, int x) {
    ReplWindow *repl = malloc(sizeof(ReplWindow));
    
    // Split into output (top) and input (bottom)
    repl->output = newwin(height-3, width, y, x);
    repl->input = newwin(3, width, y+height-3, x);
    
    box(repl->output, 0, 0);
    box(repl->input, 0, 0);
    mvwprintw(repl->input, 1, 1, "> ");
    
    scrollok(repl->output, TRUE);
    
    return repl;
}

char* repl_getline(ReplWindow *repl) {
    echo();
    mvwgetnstr(repl->input, 1, 3, repl->buffer, sizeof(repl->buffer)-1);
    noecho();
    
    // Clear input line
    wmove(repl->input, 1, 1);
    wclrtoeol(repl->input);
    mvwprintw(repl->input, 1, 1, "> ");
    box(repl->input, 0, 0);
    wrefresh(repl->input);
    
    return repl->buffer;
}
```

### Updating Values
```c
void update_status_window(WINDOW *win, int fps, int clients, double cpu) {
    // Clear content area (preserve border)
    for (int i = 1; i < getmaxy(win)-1; i++) {
        wmove(win, i, 1);
        wclrtoeol(win);
    }
    
    // Update values
    mvwprintw(win, 1, 2, "FPS:     %d", fps);
    mvwprintw(win, 2, 2, "Clients: %d", clients);
    mvwprintw(win, 3, 2, "CPU:     %.1f%%", cpu);
    
    // Redraw border (clrtoeol can damage it)
    box(win, 0, 0);
    mvwprintw(win, 0, 2, " Status ");
    
    wrefresh(win);
}
```

### Complete Example Structure
```c
int main() {
    initscr();
    cbreak();
    noecho();
    nodelay(stdscr, TRUE);  // Non-blocking input
    
    // Layout calculations
    int h = LINES;
    int w = COLS;
    int status_h = 10;
    int repl_h = 5;
    int log_h = h - status_h - repl_h;
    
    // Create windows
    WINDOW *status = newwin(status_h, w/2, 0, 0);
    WINDOW *values = newwin(status_h, w/2, 0, w/2);
    WINDOW *log = create_log_window(log_h, w, status_h, 0);
    ReplWindow *repl = create_repl(repl_h, w, status_h + log_h, 0);
    
    // Main loop
    int running = 1;
    while (running) {
        int ch = getch();
        
        if (ch == 'q') {
            running = 0;
        } else if (ch == '\n') {
            char *cmd = repl_getline(repl);
            log_message(log, cmd);
        }
        
        // Update displays
        update_status_window(status, 60, 42, 23.5);
        
        napms(50);  // 20 FPS update rate
    }
    
    endwin();
    return 0;
}
```

## Performance Considerations

### Understanding NCurses Performance

NCurses performance is primarily influenced by terminal I/O operations. The library maintains an internal screen buffer and only sends changes to the terminal, but inefficient usage can still cause performance issues.

### Key Performance Factors

1. **Terminal Bandwidth**: Serial terminals might be limited to 9600-115200 baud. Modern pseudo-terminals (PTY) are much faster but still have limits.

2. **Network Latency**: SSH connections add latency. Each refresh() causes network traffic.

3. **Terminal Emulator Processing**: Complex escape sequences take time to parse and render.

4. **Screen Complexity**: More colors, attributes, and Unicode characters increase data sent.

### Performance Best Practices

**1. Batch Updates**
```c
// Bad: Multiple refreshes
mvprintw(0, 0, "Status: OK");
refresh();
mvprintw(1, 0, "Count: %d", count);
refresh();

// Good: Single refresh
mvprintw(0, 0, "Status: OK");
mvprintw(1, 0, "Count: %d", count);
refresh();
```

**2. Use wnoutrefresh() + doupdate()**
```c
// For multiple windows
wnoutrefresh(status_win);
wnoutrefresh(log_win);
wnoutrefresh(input_win);
doupdate();  // Single terminal update
```

**3. Minimize Full Redraws**
```c
// Avoid unless necessary
clear();  // Clears entire screen
refresh();

// Better: Clear specific areas
move(5, 10);
clrtoeol();  // Clear to end of line only
```

**4. Use Pads for Large Content**
```c
// For content larger than screen
WINDOW *pad = newpad(1000, 200);  // 1000 lines
// Only refresh visible portion
prefresh(pad, scroll_y, 0, 0, 0, LINES-1, COLS-1);
```

**5. Optimize Color Usage**
```c
// Reuse color pairs
init_pair(1, COLOR_RED, COLOR_BLACK);
init_pair(2, COLOR_GREEN, COLOR_BLACK);
// Don't create new pairs for every color combination
```

### Benchmarking NCurses

```c
#include <time.h>

void benchmark_refresh_rate() {
    struct timespec start, end;
    int iterations = 1000;
    
    clock_gettime(CLOCK_MONOTONIC, &start);
    
    for (int i = 0; i < iterations; i++) {
        mvprintw(0, 0, "Frame: %d", i);
        refresh();
    }
    
    clock_gettime(CLOCK_MONOTONIC, &end);
    
    double elapsed = (end.tv_sec - start.tv_sec) + 
                    (end.tv_nsec - start.tv_nsec) / 1e9;
    double fps = iterations / elapsed;
    
    mvprintw(2, 0, "Average FPS: %.2f", fps);
    refresh();
}
```

### Performance Limits

- **Local Terminal**: Can achieve 1000+ FPS for simple updates
- **SSH (LAN)**: Typically 100-500 FPS depending on latency
- **SSH (Internet)**: Often limited to 30-60 FPS
- **Complex Unicode/Colors**: Can reduce rates by 50-90%

### Optimization Strategies

1. **Update Only Changed Areas**
```c
// Track dirty regions
typedef struct {
    int y1, x1, y2, x2;
    bool dirty;
} DirtyRegion;

void mark_dirty(DirtyRegion *region, int y, int x) {
    region->dirty = true;
    region->y1 = MIN(region->y1, y);
    region->x1 = MIN(region->x1, x);
    region->y2 = MAX(region->y2, y);
    region->x2 = MAX(region->x2, x);
}
```

2. **Frame Rate Limiting**
```c
void frame_rate_limiter(int target_fps) {
    static struct timespec last = {0};
    struct timespec now, sleep_time;
    
    clock_gettime(CLOCK_MONOTONIC, &now);
    
    long target_ns = 1000000000L / target_fps;
    long elapsed_ns = (now.tv_sec - last.tv_sec) * 1000000000L +
                      (now.tv_nsec - last.tv_nsec);
    
    if (elapsed_ns < target_ns) {
        sleep_time.tv_sec = 0;
        sleep_time.tv_nsec = target_ns - elapsed_ns;
        nanosleep(&sleep_time, NULL);
    }
    
    last = now;
}
```

3. **Async Input Handling**
```c
// Non-blocking input with select()
fd_set fds;
struct timeval tv = {0, 0};
FD_ZERO(&fds);
FD_SET(0, &fds);  // stdin

if (select(1, &fds, NULL, NULL, &tv) > 0) {
    int ch = getch();  // Input available
}
```

## NCurses Alternatives

### Terminal UI Libraries

**1. Termbox / Termbox2**
- Minimalist terminal UI library
- Simpler API than ncurses
- No dependencies
- Cross-platform (Unix/Windows)
- Good for simple TUIs

```c
#include <termbox.h>

int main() {
    tb_init();
    tb_print(0, 0, TB_WHITE, TB_BLACK, "Hello, World!");
    tb_present();
    tb_shutdown();
    return 0;
}
```

**2. Notcurses**
- Modern ncurses replacement
- Direct terminal control
- Better Unicode support
- Advanced graphics capabilities
- Higher performance

```c
#include <notcurses/notcurses.h>

int main() {
    struct notcurses* nc = notcurses_init(NULL, NULL);
    struct ncplane* n = notcurses_stdplane(nc);
    ncplane_putstr(n, "Hello, World!");
    notcurses_render(nc);
    notcurses_stop(nc);
    return 0;
}
```

**3. FTXUI (C++)**
- Modern C++ terminal UI framework
- Component-based architecture
- Reactive programming model
- Good for complex UIs

```cpp
#include <ftxui/component/component.hpp>
#include <ftxui/component/screen_interactive.hpp>

int main() {
    using namespace ftxui;
    auto screen = ScreenInteractive::Fullscreen();
    auto component = Renderer([] {
        return text("Hello, World!");
    });
    screen.Loop(component);
    return 0;
}
```

### Language-Specific Alternatives

**Python**
- **Rich**: Modern terminal rendering
- **Textual**: Reactive TUI framework
- **Urwid**: Widget-based console UI
- **Blessed**: Node.js blessed port

**Rust**
- **Ratatui** (formerly tui-rs): Immediate mode TUI
- **Cursive**: High-level TUI framework
- **Termion**: Low-level terminal control

**Go**
- **Bubble Tea**: Elm-inspired TUI framework
- **Termui**: Dashboard-focused library
- **Tcell**: Lower-level terminal access

**JavaScript/Node.js**
- **Blessed**: Comprehensive TUI framework
- **Ink**: React for CLIs
- **Terminal-kit**: Full-featured terminal lib

### Direct Terminal Control

**ANSI Escape Sequences**
```c
// No library needed
#include <stdio.h>

#define CLEAR_SCREEN "\033[2J\033[H"
#define MOVE_CURSOR(y,x) printf("\033[%d;%dH", (y), (x))
#define SET_COLOR(fg,bg) printf("\033[%d;%dm", (fg), (bg))

int main() {
    printf(CLEAR_SCREEN);
    MOVE_CURSOR(10, 20);
    SET_COLOR(31, 40);  // Red on black
    printf("Hello, World!");
    SET_COLOR(0, 0);    // Reset
    fflush(stdout);
    return 0;
}
```

### Choosing an Alternative

**Use NCurses when:**
- Need maximum compatibility
- Working with legacy code
- Need extensive documentation
- Want a proven, stable solution

**Use Termbox when:**
- Want minimal dependencies
- Need simple, clean API
- Building basic TUIs
- Cross-platform is important

**Use Notcurses when:**
- Need high performance
- Want modern features
- Building media-rich TUIs
- Don't need legacy support

**Use ANSI sequences when:**
- Ultra-minimal requirements
- Full control needed
- Building simple utilities
- Learning terminal internals

**Use language-specific libs when:**
- Working in non-C languages
- Want idiomatic solutions
- Need ecosystem integration
- Building complex applications

### Migration from NCurses

Example migration to Termbox:
```c
// NCurses
initscr();
mvprintw(10, 20, "Hello");
refresh();
getch();
endwin();

// Termbox equivalent
tb_init();
tb_print(20, 10, TB_DEFAULT, TB_DEFAULT, "Hello");
tb_present();
tb_poll_event(&event);
tb_shutdown();
```

## Additional Resources

- [NCurses HOWTO](https://tldp.org/HOWTO/NCURSES-Programming-HOWTO/)
- [NCurses Man Pages](https://invisible-island.net/ncurses/man/)
- [PDCurses for Windows](https://pdcurses.org/)
- [NCurses Programming Guide](https://invisible-island.net/ncurses/ncurses-intro.html)
- [Termbox GitHub](https://github.com/termbox/termbox)
- [Notcurses GitHub](https://github.com/dankamongmen/notcurses)
- [Terminal Control Sequences](https://www.xfree86.org/current/ctlseqs.html)