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
