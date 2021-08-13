# rokoel-website

Compile with gcc:
  execute from ../Core:
  ```bash
    gcc -pthread core.c server.c socket.c secondary.c -o ../Builds/serv -fPIC
  ```

Execute:
  execute from ../Core:
  ```bash
    sudo ../Builds/serv
  ```


WORKS ON LINUX ONLY (socket.h is available only there)

Planning to make Redis main database in the future.

Script that makes creating new routes easier may be created in the future (shell, python or C).

We are not using hitch now due to its bugginess (or my hands are crooked(: ). We are making our website http and use Cloudflare instead (same result but better reliability). So there is no way to use https locally, unfortunately.
