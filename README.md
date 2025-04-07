# Sharded-File-Storage
Uni project. Client-Server C++ app for storing files remotely.

Everything is stored on the server machine.
Implements a simple load balancing mechanism by sending data into two separate folders (StorageServer 1/2). Creating a file means splitting it into two shards (in this case but can be modified). Getting a file's content from server means reuniting the two shards and sending the content. When requesting a file, the client machine opens Notepad and displays the content in a local temp.txt file. Deleting a stored means deleting its shards from the storage servers.

!IMPORTANT!
-> The app requires ODBC driver. The database used to store users' credentials is called UsersFileExplorer in this case. The main Server connects to the DB via connection string.
-> Only works with .txt files
