Implemented the external Unix V6 File System that supports the file size upto 4gb using C and Linux system commands. The newly designed file system has the block size of 1024 kb, the Inode size of 64 kb along with with modified I-node structure. Follwing commands are implemented for the newly designed file system  

initfs <Name of the file system>  <Number of blocks> <Number of I-nodes> => command to Initialise the file system

(a) cpin <externalfile> <v6-file>  => copy in the contents of external file to v6-file
(b) cpout <v6-file> <externalfile> => copy out the contents of v6-file to externalfile
(c) mkdir <v6-directory> => create a new directory
(d) rm <v6-file> => remove the file
(e) ls => list the contents of the directory
(f) pwd => show current working directory
(g) cd <dirname> => change directory
(h) rmdir <directory> => remove the directory
(i) open <filename> => open the file