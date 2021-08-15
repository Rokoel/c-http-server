# rokoel-website

Compile with gcc:
  execute from ../Core:
  ```bash
    gcc -pthread core.c server.c socket.c secondary.c -o ../Builds/serv -fPIC -fno-stack-protector
  ```

-fno-stack-protector is used because in place, where buffer overflow occurs, we handle it by ourselves so nothing is being damaged. If we don't include this flag, "clever" gcc will try to say that this is dangerous and will drop the program.

Execute:
  execute from ../Core:
  ```bash
    sudo ../Builds/serv
  ```


WORKS ON LINUX ONLY (socket.h is available only there)

Planning to make Redis main database in the future.

Script that makes creating new routes easier may be created in the future (shell, python or C).

Combining parts of documents in one on the server side may be added in the future.

We are not using hitch now due to its bugginess (or my hands are crooked(: ). We are making our website http and use Cloudflare instead (same result but better reliability). So there is no way to use https locally, unfortunately.
