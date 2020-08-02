#include <sys/ptrace.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/reg.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <stdio.h>
#include <vector>
#include <map>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <iterator>

using namespace std;

const int long_size = sizeof(long);
char * f_name;
int global_counter = 1;

void reverse(char * str) {
    int i, j;
    char temp;
    for (i = 0, j = strlen(str) - 1; i <= j; ++i, --j) {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
    }
}

void transform_file_name(char * str, char * tmp_name, char * save_name, char * bkdir_path) {
    tmp_name[0] = '\0';
    strcpy(tmp_name, str);
    str[0] = '\0';
    int ind = 0;
    for (int i = strlen(tmp_name) - 1; tmp_name[i] != '/'; i--) {
        str[ind++] = tmp_name[i];
    }
    
    str[ind] = '\0';
    reverse(str);
    str[strlen(str)] = '\0';
    
    char extension[100];
    int i = 0, j = 0;
    ind = 0;
    extension[0] = '\0';
    for (i = 0; str[i] != '\0'; i++) {
        if (str[i] == '.') {
            break;
        }
    }
    for (j = i; str[j] != '\0'; j++) {
        extension[ind++] = str[j];
    }
    if (i != strlen(str)) {
        str[i] = '\0';
    }
    extension[ind] = '\0';
    strcpy(save_name, bkdir_path);
    strcat(save_name, str);
    strcat(save_name, "_");
    char counter[255];
    global_counter++;
    sprintf(counter, "%d", global_counter);
    strcat(save_name, counter);
    strcat(save_name, "_");
    //appending timestamp(for versioning)
    char version[255];
    time_t curtime;
    time( & curtime);
    sprintf(version, "%s", ctime( & curtime));
    char * temp = version;
    for (int i=0; i<strlen(temp); i++)
    {
       if (temp[i] == ' '){
          temp[i] = '_'; 
       }
         
    }
    version[strlen(version)-1] = '\0';
    strcat(save_name, version);
    strcat(save_name, extension);
    //cout << "NEW FILE NAME: \n" << save_name << endl;
}

void getdata(pid_t child, long addr, char * str, int len) {
    char * laddr;
    int i, j;
    union u {
        long val;
        char chars[long_size];
    }
    data;
    i = 0;
    j = len / long_size;
    laddr = str;
    while (i < j) {
        data.val = ptrace(PTRACE_PEEKDATA, child, addr + i * 4, NULL);
        memcpy(laddr, data.chars, long_size);
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if (j != 0) {
        data.val = ptrace(PTRACE_PEEKDATA, child, addr + i * 4, NULL);
        memcpy(laddr, data.chars, j);
    }
    str[len] = '\0';
}

void putdata(pid_t child, long addr, char * str, int len) {
    char * laddr;
    int i, j;
    union u {
        long val;
        char chars[long_size];
    }
    data;
    i = 0;
    j = len / long_size;
    laddr = str;
    while (i < j) {
        memcpy(data.chars, laddr, long_size);
        ptrace(PTRACE_POKEDATA, child, addr + i * 4, data.val);
        ++i;
        laddr += long_size;
    }
    j = len % long_size;
    if (j != 0) {
        memcpy(data.chars, laddr, j);
        ptrace(PTRACE_POKEDATA, child, addr + i * 4, data.val);
    }
}

int main(int argc, char* argv[]) {
    
    pid_t child;
    pid_t child2;
    char * tmp_name;
    char * save_name;
    char bkdir_path[500];
    strcpy(bkdir_path, "/home/sekar/bk/");
    map < int, char * > f_record;
    child = fork();
    if (child == 0) {
        ptrace(PTRACE_TRACEME, 0, NULL, NULL);
        execvp(argv[1], &argv[1]);
        //execl("/home/sekar/A2/test_ans", "test_ans", NULL);
    } else {
        long orig_eax;
        long params[3];
        int status;
        char * str;
        char * laddr;
        int toggle = 0;
        while (1) {
            wait( &status);
            if (WIFEXITED(status))
                break;
            orig_eax = ptrace(PTRACE_PEEKUSER, child, 4 * ORIG_EAX, NULL);
            if (orig_eax == SYS_open || orig_eax == SYS_creat) {
                if (toggle == 0) {
                    toggle = 1;
                    params[0] = ptrace(PTRACE_PEEKUSER, child, 4 * EBX, NULL);
                    params[1] = ptrace(PTRACE_PEEKUSER, child, 4 * ECX, NULL);
                    params[2] = ptrace(PTRACE_PEEKUSER, child, 4 * EDX, NULL);

                    // check whether write bit is set
                    int check_write_bit = (params[1] >> 1) & 1;

                    str = (char * ) malloc((500 + 1) *
                        sizeof(char));
                    tmp_name = (char * ) malloc((500 + 1) *
                        sizeof(char));
                    save_name = (char * ) malloc((500 + 1) *
                        sizeof(char));
                    f_name = (char * ) malloc((500 + 1) *
                        sizeof(char));
                    
                    //get original file name
                    getdata(child, params[0], str, 500);
                    //f_name keeps the original file name so that it can be inserted in the map with its file descriptor
                    
                    f_name[0] = '\0';
                    strcpy(f_name, str);
                    transform_file_name(str, tmp_name, save_name, bkdir_path);
                    
                    //if write bit is set then create a backup
                    
                    if (check_write_bit) {
                        child2 = fork();
                        if (child2 == 0) {
                            mkdir(bkdir_path, 0777);
                            cout << "File is being saved with name: \n" << save_name << endl;
                            execl("/bin/cp", "-i", "-p", tmp_name, save_name, (char * ) 0);
                        }
                        //else{
                        //    wait(&status);
                        //}

                    }

                } else {
                    int desc = ptrace(PTRACE_PEEKUSER, child, 4 * EAX, NULL);
                    // store file name and file descrptor in a map
                    if (desc >= 3) {
                        f_record[desc] = f_name;
                    }

                    toggle = 0;
                }
            }
            
            if (orig_eax == SYS_write) {
                if (toggle == 0) {
                    toggle = 1;
                    params[0] = ptrace(PTRACE_PEEKUSER, child, 4 * EBX, NULL);
                    params[1] = ptrace(PTRACE_PEEKUSER, child, 4 * ECX, NULL);
                    params[2] = ptrace(PTRACE_PEEKUSER, child, 4 * EDX, NULL);

                    // checking for file descriptor  >= 3 so that we dont create backups of input/output/error stream

                    if (params[0] >= 3) {
                        str[0] = '\0';
                        // fetching file name from the map corresponding to the file descriptor
                        strcpy(str, f_record[int(params[0])]);
                        transform_file_name(str, tmp_name, save_name, bkdir_path);
                        
                        child2 = fork();
                        if (child2 == 0) {
                            mkdir(bkdir_path, 0777);
                            cout << "File is being saved with name: \n" << save_name << endl;
                            execl("/bin/cp", "-i", "-p", tmp_name, save_name, (char * ) 0);
                        } else {
                            wait(&status);
                        }
                    }

                } else {
                    toggle = 0;
                }
            }
            
            if (orig_eax == SYS_rename) {
                if (toggle == 0) {
                    toggle = 1;
                    params[0] = ptrace(PTRACE_PEEKUSER, child, 4 * EBX, NULL);
                    params[1] = ptrace(PTRACE_PEEKUSER, child, 4 * ECX, NULL);
                    params[2] = ptrace(PTRACE_PEEKUSER, child, 4 * EDX, NULL);

                    //keep original file name in str
                    getdata(child, params[0], str, 500);
                    //keep original file name in f_name
                    getdata(child, params[1], f_name, 500);
                    //finding file descriptor associated with the original file name from the map
                    int desc_asscociated;
                    std::map < int, char * > ::iterator it;
                    for (it = f_record.begin(); it != f_record.end(); it++) {
                        if (strcmp(it->second, str) == 0) {
                            desc_asscociated = it->first;
                            break;
                        }
                    }
                    // updating with the new file name in the map
                    f_record[desc_asscociated] = f_name;

                    transform_file_name(str, tmp_name, save_name, bkdir_path);
                    //saving the backup of the original file
                    strcpy(tmp_name, f_name);
                    
                    child2 = fork();
                    if (child2 == 0) {
                        mkdir(bkdir_path, 0777);
                        cout << "File is being saved with name: \n" << save_name << endl;
                        execl("/bin/cp", "-i", "-p", tmp_name, save_name, (char * ) 0);
                    } else {
                        wait(&status);
                    }

                } else {
                    toggle = 0;
                }
            }
            if (orig_eax == SYS_truncate) {
                if (toggle == 0) {
                    toggle = 1;
                    params[0] = ptrace(PTRACE_PEEKUSER, child, 4 * EBX, NULL);
                    params[1] = ptrace(PTRACE_PEEKUSER, child, 4 * ECX, NULL);
                    params[2] = ptrace(PTRACE_PEEKUSER, child, 4 * EDX, NULL);

                    getdata(child, params[0], str, 500);
                    transform_file_name(str, tmp_name, save_name, bkdir_path);
                    
                    child2 = fork();
                    if (child2 == 0) {
                        mkdir(bkdir_path, 0777);
                        cout << "File is being saved with name: \n" << save_name << endl;
                        execl("/bin/cp", "-i", "-p", tmp_name, save_name, (char * ) 0);
                    } else {

                        wait(&status);
                    }

                } else {
                    toggle = 0;
                }
            }
            if (orig_eax == SYS_ftruncate) {
                if (toggle == 0) {
                    toggle = 1;
                    params[0] = ptrace(PTRACE_PEEKUSER, child, 4 * EBX, NULL);
                    params[1] = ptrace(PTRACE_PEEKUSER, child, 4 * ECX, NULL);
                    params[2] = ptrace(PTRACE_PEEKUSER, child, 4 * EDX, NULL);

                    // checking for file descriptor  >= 3 so that we dont create backups of input/output/error stream

                    if (params[0] >= 3) {
                        str[0] = '\0';
                        // fetching file name from the map corresponding to the file descriptor
                        strcpy(str, f_record[int(params[0])]);
                        transform_file_name(str, tmp_name, save_name, bkdir_path);
                        
                        child2 = fork();
                        if (child2 == 0) {
                            mkdir(bkdir_path, 0777);
                            cout<<"here";
                            cout << "File is being saved with name: \n" << save_name << endl;
                            execl("/bin/cp", "-i", "-p", tmp_name, save_name, (char * ) 0);
                        } else {
                            wait(&status);
                        }
                    }

                } else {
                    toggle = 0;
                }
            }

            ptrace(PTRACE_SYSCALL, child, NULL, NULL);
        }
    }
    return 0;
}