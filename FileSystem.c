//File Name: FileSystem.c
//Team Members: Rupesh Biradar and Pragya Nagpal
//UTD_ID: 2021463884 and 2021489743
//NetID: rrb180004 and pxn190012
//Class: OS 5348.001
//Project: Project 
          

// Objective: To design a modified version of Unix V6 file-system with
// 1024 bytes of block size and 4 GB max file size.
// To execute program, the following commands can be used on Linux server:
// gcc FileSystem.c -o FileSystem.c
// ./FileSystem.c
//
// ***********************************************************************
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <ctype.h>
#include <errno.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define BLOCK_SIZE 1024
#define MAX_FILE_SIZE 4194304 // 4GB of file size
#define FREE_ARRAY_SIZE 248 // free and inode array size
#define INODE_SIZE 64


/*************** super block structure**********************/
typedef struct {
        unsigned int isize; // 4 byte
        unsigned int fsize;
        unsigned int nfree;
        unsigned short free[FREE_ARRAY_SIZE];
        unsigned int ninode;
        unsigned short inode[FREE_ARRAY_SIZE];
        unsigned short flock;
        unsigned short ilock;
        unsigned short fmod;
        unsigned int time[2];
} super_block;

/****************inode structure ************************/
typedef struct {
        unsigned short flags; // 2 bytes
        char nlinks;  // 1 byte
        char uid;
        char gid;
        unsigned int size; // 32bits  2^32 = 4GB filesize
        unsigned short addr[22]; // to make total size = 64 byte inode size
        unsigned int actime;
        unsigned int modtime;
} Inode;

typedef struct
{
        unsigned short inode;
        char filename[14];
}dEntry;

super_block super;
int fd;
char pwd[100];
int curINodeNumber;
char fileSystemPath[100];
int total_inodes_count;

void writeToBlock (int blockNumber, void * buffer, int nbytes)
{
        lseek(fd,BLOCK_SIZE * blockNumber, SEEK_SET);
        write(fd,buffer,nbytes);
}


void writeToBlockOffset(int blockNumber, int offset, void * buffer, int nbytes)
{
        lseek(fd,(BLOCK_SIZE * blockNumber) + offset, SEEK_SET);
        write(fd,buffer,nbytes);
}

void readFromBlockOffset(int blockNumber, int offset, void * buffer, int nbytes)
{
        lseek(fd,(BLOCK_SIZE * blockNumber) + offset, SEEK_SET);
        read(fd,buffer,nbytes);
}


void addFreeBlock(int blockNumber){
        if(super.nfree == FREE_ARRAY_SIZE)
        {
                //write to the new block
                writeToBlock(blockNumber, super.free, FREE_ARRAY_SIZE * 2);
                super.nfree = 0;
        }
        super.free[super.nfree] = blockNumber;
        super.nfree++;
}

int getFreeBlock(){
        if(super.nfree == 0){
                int blockNumber = super.free[0];
                lseek(fd,BLOCK_SIZE * blockNumber , SEEK_SET);
                read(fd,super.free, FREE_ARRAY_SIZE * 2);
                super.nfree = 100;
                return blockNumber;
        }
        super.nfree--;
        return super.free[super.nfree];
}

void addFreeInode(int iNumber){
        if(super.ninode == FREE_ARRAY_SIZE)
                return;
        super.inode[super.ninode] = iNumber;
        super.ninode++;
}

Inode getInode(int INumber){
        Inode iNode;
        int blockNumber = (INumber * INODE_SIZE) / BLOCK_SIZE;    // need to remove 
        int offset = (INumber * INODE_SIZE) % BLOCK_SIZE;
        lseek(fd,(BLOCK_SIZE * blockNumber) + offset, SEEK_SET);
        read(fd,&iNode,INODE_SIZE);
        return iNode;
}

int getFreeInode(){
        if (super.ninode <= 0) {
            int i;
            for (i = 2; i<total_inodes_count; i++) {
                Inode freeInode = getInode(super.inode[i]);
                if (freeInode.flags != 1<<15) {
                    super.inode[super.ninode] = i;
                    super.ninode++;
                }
            }
        }
        super.ninode--;
        return super.inode[super.ninode];
}

void writeInode(int INumber, Inode inode){
        int blockNumber = (INumber * INODE_SIZE )/ BLOCK_SIZE;   //need to remove
        int offset = (INumber * INODE_SIZE) % BLOCK_SIZE;
        writeToBlockOffset(blockNumber, offset, &inode, sizeof(Inode));
}

void createRootDirectory(){
        int blockNumber = getFreeBlock();
        dEntry directory[2];
        directory[0].inode = 0;
        strcpy(directory[0].filename,".");

        directory[1].inode = 0;
        strcpy(directory[1].filename,"..");

        writeToBlock(blockNumber, directory, 2*sizeof(dEntry));

        Inode root;
        root.flags = 1<<14 | 1<<15; // setting 14th and 15th bit to 1, 15: allocated and 14: directory
        root.nlinks = 1;
        root.uid = 0;
        root.gid = 0;
        root.size = 2*sizeof(dEntry);
        root.addr[0] = blockNumber;
        root.actime = time(NULL);
        root.modtime = time(NULL);

        writeInode(0,root);
        curINodeNumber = 0;
        strcpy(pwd,"/");
}

void ls(){                                                              // list directory contents
        Inode curINode = getInode(curINodeNumber);
        int blockNumber = curINode.addr[0];
        dEntry directory[100];
        int i;
        readFromBlockOffset(blockNumber,0,directory,curINode.size);
        for(i = 0; i < curINode.size/sizeof(dEntry); i++)
        {
                printf("%s\n",directory[i].filename);
        }
}

void makedir(char* dirName)
{
        int blockNumber = getFreeBlock(); // block to store directory table
        int iNumber = getFreeInode(); // inode numbr for directory
        dEntry directory[2];
        directory[0].inode = iNumber;
        strcpy(directory[0].filename,".");
        printf("%s",directory[0].filename);

        directory[1].inode = curINodeNumber;
        strcpy(directory[1].filename,"..");
        printf("%s",directory[1].filename);

        writeToBlock(blockNumber, directory, 2*sizeof(dEntry));
// write directory i node
        Inode dir;
        dir.flags = 1<<14 | 1<<15; // setting 14th and 15th bit to 1, 15: allocated and 14: directory
        dir.nlinks = 1;
        dir.uid = 0;
        dir.gid = 0;
        dir.size = 2*sizeof(dEntry);
        dir.addr[0] = blockNumber;
        dir.actime = time(NULL);
        dir.modtime = time(NULL);

        writeInode(iNumber,dir);

// update pRENT DIR I NODE
        Inode curINode = getInode(curINodeNumber);
        blockNumber = curINode.addr[0];
        dEntry newDir;
        newDir.inode = iNumber ;
        strcpy(newDir.filename,dirName);
        int i;
        writeToBlockOffset(blockNumber,curINode.size,&newDir,sizeof(dEntry));
        curINode.size += sizeof(dEntry);
        writeInode(curINodeNumber,curINode);
}

int lastIndexOf(char s[100], char ch) {
    int i, p=-1;
    for (i=0; i<100; i++) {
        if (s[i]==ch)
            p = i;
    }
    return p;
}

void slice_str(const char * str, char * buffer, size_t start, size_t end)
{
    size_t j = 0;
    size_t i;
    for ( i = start; i <= end; ++i ) {
        buffer[j++] = str[i];
    }
    buffer[j] = 0;
}

void changedir(char* dirName)
{
        Inode curINode = getInode(curINodeNumber);
        int blockNumber = curINode.addr[0];
        dEntry directory[100];
        int i;
        readFromBlockOffset(blockNumber,0,directory,curINode.size);
        for(i = 0; i < curINode.size/sizeof(dEntry); i++)
        {
                if(strcmp(dirName,directory[i].filename)==0){
                        Inode dir = getInode(directory[i].inode);
                        if(dir.flags ==( 1<<14 | 1<<15)){
                                if (strcmp(dirName, ".") == 0) {
                                        return;
                                }
                                else if (strcmp(dirName, "..") == 0) {
                                        curINodeNumber=directory[i].inode;
                                        int lastSlashPosition = lastIndexOf(pwd, '/');
                                        char temp[100];
                                        slice_str(pwd, temp, 0, lastSlashPosition-1);
                                        strcpy(pwd, temp);
                                }
                                else {
                                        curINodeNumber=directory[i].inode;
                                        strcat(pwd, "/");
                                        strcat(pwd, dirName);
                                } 
                        }
                        else{
                                printf("\n%s\n","NOT A DIRECTORY!");
                        }
                        return;
                }
        }
}

void copyIn(char* sourcePath, char* filename){
        int source,blockNumber;
        if((source = open(sourcePath,O_RDWR|O_CREAT,0600))== -1)
        {
                printf("\n file opening error [%s]\n",strerror(errno));
                return;
        }

        int iNumber = getFreeInode();
        Inode file;
        file.flags = 1<<15; // setting 15th bit to 1, 15: allocated
        file.nlinks = 1;
        file.uid = 0;
        file.gid = 0;
        file.size = 0;
        file.actime = time(NULL);
        file.modtime = time(NULL);
// reAD source and copy to desti, block by block
        int bytesRead = BLOCK_SIZE;
        char buffer[BLOCK_SIZE] = {0};
        int i = 0;
        while(bytesRead == BLOCK_SIZE){
                bytesRead = read(source,buffer,BLOCK_SIZE);
                file.size += bytesRead;
                blockNumber = getFreeBlock();
                file.addr[i++] = blockNumber;
                writeToBlock(blockNumber, buffer, bytesRead);
        }
        writeInode(iNumber,file);

        Inode curINode = getInode(curINodeNumber);
        blockNumber = curINode.addr[0];
        dEntry newFile;
        newFile.inode = iNumber ;
        strcpy(newFile.filename,filename);
        writeToBlockOffset(blockNumber,curINode.size,&newFile,sizeof(dEntry));
        curINode.size += sizeof(dEntry);
        writeInode(curINodeNumber,curINode);
}


void copyOut(char* destinationPath, char* filename){
        int dest,blockNumber,x,i;
        char buffer[BLOCK_SIZE] = {0};
        if((dest = open(destinationPath,O_RDWR|O_CREAT,0600))== -1)
        {
                printf("\n file opening error [%s]\n",strerror(errno));
                return;
        }

        Inode curINode = getInode(curINodeNumber);
        blockNumber = curINode.addr[0];
        dEntry directory[100];
        readFromBlockOffset(blockNumber,0,directory,curINode.size);
        for(i = 0; i < curINode.size/sizeof(dEntry); i++)
        {
                if(strcmp(filename,directory[i].filename)==0){
                        Inode file = getInode(directory[i].inode);
	unsigned short* source = file.addr;
                        if(file.flags ==(1<<15)){
                                for(x = 0; x<file.size/BLOCK_SIZE; x++)
                                {
                                        blockNumber = source[x];
                                        readFromBlockOffset(blockNumber, 0, buffer, BLOCK_SIZE);
                                        write(dest,buffer,BLOCK_SIZE);
                                }
                                blockNumber = source[x];
                                readFromBlockOffset(blockNumber, 0, buffer, file.size%BLOCK_SIZE);
                                write(dest,buffer,file.size%BLOCK_SIZE);

                        }
                        else{
                                printf("\n%s\n","NOT A FILE!");
                        }
                        return;
                }
        }
}

void rm(char* filename){
        int blockNumber,x,i;
        Inode curINode = getInode(curINodeNumber);
        blockNumber = curINode.addr[0];
        dEntry directory[100];
        readFromBlockOffset(blockNumber,0,directory,curINode.size);

        for(i = 0; i < curINode.size/sizeof(dEntry); i++)
        {
                if(strcmp(filename,directory[i].filename)==0){
                        Inode file = getInode(directory[i].inode);
                        if(file.flags ==(1<<15)){
                                for(x = 0; x<file.size/BLOCK_SIZE; x++)
                                {
                                        blockNumber = file.addr[x];
                                        addFreeBlock(blockNumber);
                                }
                                if(0<file.size%BLOCK_SIZE){
                                        blockNumber = file.addr[x];
                                        addFreeBlock(blockNumber);
                                }
                                addFreeInode(directory[i].inode);
                                directory[i]=directory[(curINode.size/sizeof(dEntry))-1];
                                curINode.size-=sizeof(dEntry);
                                writeToBlock(curINode.addr[0],directory,curINode.size);
                                writeInode(curINodeNumber,curINode);
                        }
                        else{
                                printf("\n%s\n","NOT A FILE!");
                        }
                        return;
                }
        }

}

void removeDir(char* filename){
        int blockNumber,x,i;
        Inode curINode = getInode(curINodeNumber);
        blockNumber = curINode.addr[0];
        dEntry directory[100];
        readFromBlockOffset(blockNumber,0,directory,curINode.size);

        for(i = 0; i < curINode.size/sizeof(dEntry); i++)
        {
                if(strcmp(filename,directory[i].filename)==0){
                        Inode file = getInode(directory[i].inode);
                         if(file.flags ==( 1<<14 | 1<<15)){
                                for(x = 0; x<file.size/BLOCK_SIZE; x++)
                                {
                                        blockNumber = file.addr[x];
                                        addFreeBlock(blockNumber);
                                }
                                if(0<file.size%BLOCK_SIZE){
                                        blockNumber = file.addr[x];
                                        addFreeBlock(blockNumber);
                                }
                                addFreeInode(directory[i].inode);
                                directory[i]=directory[(curINode.size/sizeof(dEntry))-1];
                                curINode.size-=sizeof(dEntry);
                                writeToBlock(curINode.addr[0],directory,curINode.size);
                                writeInode(curINodeNumber,curINode);
                            }
                         else{
                                printf("\n%s\n","NOT A DIRECTORY!");
                         }
                        return;
                }
        }

}
int openfs(const char *filename)
{
	fd=open(filename,2);
	lseek(fd,BLOCK_SIZE,SEEK_SET);
	read(fd,&super,sizeof(super));
	lseek(fd,2*BLOCK_SIZE,SEEK_SET);
        Inode root = getInode(1);
	read(fd,&root,sizeof(root));
	return 1;
}
void initfs(char* path, int total_blocks, int total_inodes)
{
        printf("\n filesystem intialization started \n");
        total_inodes_count = total_inodes;
        char emptyBlock[BLOCK_SIZE] = {0};
        int no_of_bytes,i,blockNumber,iNumber;

        //init isize (Number of blocks for inode
        if(((total_inodes*INODE_SIZE)%BLOCK_SIZE) == 0) // 300*64 % 1024
                super.isize = (total_inodes*INODE_SIZE)/BLOCK_SIZE;
        else
                super.isize = (total_inodes*INODE_SIZE)/BLOCK_SIZE+1;

        //init fsize
        super.fsize = total_blocks;

        //create file for File System
        if((fd = open(path,O_RDWR|O_CREAT,0600))== -1)
        {
                printf("\n file opening error [%s]\n",strerror(errno));
                return;
        }
        strcpy(fileSystemPath,path);

        writeToBlock(total_blocks-1,emptyBlock,BLOCK_SIZE); // writing empty block to last block

        // add all blocks to the free array
        super.nfree = 0;
        for (blockNumber= 1+super.isize; blockNumber< total_blocks; blockNumber++)
                addFreeBlock(blockNumber);

        // add free Inodes to inode array
        super.ninode = 0;
        for (iNumber=1; iNumber < total_inodes ; iNumber++)
                addFreeInode(iNumber);


        super.flock = 'f';
        super.ilock = 'i';
        super.fmod = 'f';
        super.time[0] = 0;
        super.time[1] = 0;

        //write Super Block
        writeToBlock (0,&super,BLOCK_SIZE);

        //allocate empty space for i-nodes
        for (i=1; i <= super.isize; i++)
                writeToBlock(i,emptyBlock,BLOCK_SIZE);

        createRootDirectory();
}

void quit()
{
        close(fd);
        exit(0);
}


int main(int argc, char *argv[])
{
        char c;

        printf("\n Clearing screen \n");
        system("clear");

        unsigned int blk_no =0, inode_no=0;
        char *fs_path;
        char *arg1, *arg2;
        char *my_argv, cmd[512];
                printf("\nSupported Commands:\n");
                printf("\n1: initfs <File_System_Name> <Number_of_Blocks> <Number_of_Inodes>\n");
                printf("\n2: mkdir <Directory_Name>\n");
                printf("\n3: cd <Directory_Name>\n");
                printf("\n4: ls\n");
                printf("\n5: cd <Directory_Name>\n");
                printf("\n6: cpin <External_File> <V6_File>\n");
                printf("\n7: cpout  <Destination_External_File> <Source_V6_File>\n");
                printf("\n8: rm  <File_Name>\n");
                printf("\n8: rmdir  <Directory_Name>\n");
        while(1)
        {
                printf("\n%s@%s>>>",fileSystemPath,pwd);
                
                scanf(" %[^\n]s", cmd);
                my_argv = strtok(cmd," ");

                if(strcmp(my_argv, "initfs")==0)
                {

                        fs_path = strtok(NULL, " ");
                        arg1 = strtok(NULL, " ");
                        arg2 = strtok(NULL, " ");
                        if(access(fs_path, X_OK) != -1)
                        {
                                printf("filesystem already exists. \n");
                                printf("same file system will be used\n");
                        }
                        else
                        {
                                if (!arg1 || !arg2)
                                        printf(" insufficient arguments to proceed\n");
                                else
                                {
                                        blk_no = atoi(arg1);
                                        inode_no = atoi(arg2);
                                        initfs(fs_path,blk_no, inode_no);
                                }
                        }
                        my_argv = NULL;
                }
                else if(strcmp(my_argv, "q")==0){
                        quit();
                }
                else if(strcmp(my_argv, "ls")==0){
                        ls();
                }
                else if(strcmp(my_argv, "mkdir")==0){
                        arg1 = strtok(NULL, " ");
                        makedir(arg1);
                }
                else if(strcmp(my_argv, "cd")==0){
                        arg1 = strtok(NULL, " ");
                        changedir(arg1);
                }
                else if(strcmp(my_argv, "cpin")==0){
                        arg1 = strtok(NULL, " ");
                        arg2 = strtok(NULL, " ");
                        copyIn(arg1,arg2);
                }
                else if(strcmp(my_argv, "cpout")==0){
                        arg1 = strtok(NULL, " ");
                        arg2 = strtok(NULL, " ");
                        copyOut(arg1,arg2);
                }
                else if(strcmp(my_argv, "rm")==0){
                        arg1 = strtok(NULL, " ");
                        rm(arg1);
                }
                else if(strcmp(my_argv, "rmdir")==0){
                        arg1 = strtok(NULL, " ");
                        removeDir(arg1);
                }else if(strcmp(my_argv, "open")==0){
                        arg1 = strtok(NULL, " ");
                        openfs(arg1);
                }
                else if(strcmp(my_argv, "pwd")==0){
                        printf("%s\n",pwd);
                }
               

        }
}