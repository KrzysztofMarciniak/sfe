# ALPSC
*Pronounced [/ælps/](https://dictionary.cambridge.org/pronunciation/english/alps)*

**Climb higher with less weight.**

ALPSC is a minimalist web development stack built for **speed**, **simplicity**, and **power**. It combines lightweight, battle-tested tools to empower **advanced developers** to swiftly create lean web applications without the bloat of modern frameworks.

- **A** – [Alpine.js](https://alpinejs.dev/) & [Alpine Linux](https://www.alpinelinux.org/): Alpine.js provides a reactive, lightweight JavaScript framework for dynamic frontends, while Alpine Linux is the minimal OS that powers the stack at the system level.
- **L** – [Lighttpd](https://www.lighttpd.net/): Fast, low-memory web server optimized for performance with simple configuration.
- **P** – [Pico.css](https://picocss.com/): Minimal CSS framework for clean and fast frontend styling (does not rely on classes as heavily as Tailwind!).
- **S** – [SQLite3](https://sqlite.org/): Embedded, serverless database for fast, reliable data storage. KISS.
- **C** – C language: High-performance backend logic via CGI scripts for ultimate control.

## Why ALPSC?
- **Fast to run**: C and Lighttpd deliver raw speed, with SQLite’s in-process design ensuring low-latency data access.
- **Fast to develop**: Simple tools like Alpine.js and Pico.css enable rapid prototyping without complex build systems.
- **Powerful**: Fine-grained control with C, SQLite, and Lighttpd.
  
## How do I test it?
This repository includes a sample web app that tracks website visits and the date of the most recent visit using an SQLite database. A button on the page increments the visit counter, updating the database via a C-based CGI script.

To test the app:
1. Clone the repository: `git clone https://github.com/KrzysztofMarciniak/ALPSC`
2. Navigate to the project directory: `cd ALPSC`
3. Start the app with Docker Compose: `docker compose up`
4. Open your browser and visit `http://localhost:80`
5. Click the button to increment the visit counter and verify the updated count and date in the UI.
   
![logo](imgs/ALPSC.png)
