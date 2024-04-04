#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/mount.h>
#include <sys/time.h>

const int TEST_ITERATIONS=10;
const int OP_ITERATIONS=1000;
const char *MOUNT_POINT = "/mnt/pmem";
const char *OUTPUT_DIR = "output-temp";
const int NAME_LEN = 128;
const int SMALL_BUFFER_SIZE = 1024;
const int MED_BUFFER_SIZE = 1024*16;
const int BIG_BUFFER_SIZE = 1024*64;

int mountfs(char* mount_type) {
    char *mount_opts, *command;
    if (strcmp(mount_type, "ext4") == 0) {
        mount_opts = "dax";
        command = "yes | mkfs.ext4 /dev/pmem0";
        system(command);
    } else {
        mount_opts = "init";
    }
    int ret = mount("/dev/pmem0", MOUNT_POINT, mount_type, 0, mount_opts);
    if (ret < 0) {
        perror("mount");
        return ret;
    }
    return 0;
}

int unmountfs() {
    int ret = umount(MOUNT_POINT);
    if (ret < 0) {
        perror("umount");
        return ret;
    }
}

long calculate_latency(struct timeval tv_start, struct timeval tv_end) {
    unsigned long start = (unsigned long)tv_start.tv_sec * 1000000 + (unsigned long)tv_start.tv_usec;
    unsigned long end = (unsigned long)tv_end.tv_sec * 1000000 + (unsigned long)tv_end.tv_usec;
    return end - start;
}

int measure_mkdir(char *mount_type, char *output_path) {
    int ret;
    FILE* fp;
    char *file_name = "dir";
    char target[64];
    char output_file[NAME_LEN];
    struct timeval tv_start, tv_end;
    unsigned long latency;

    printf("Measuring mkdir latency\n");

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        ret = mountfs(mount_type);
        if (ret < 0) {
            return ret;
        }
        sprintf(output_file, "%s/mkdir/Run%d", output_path, i);
        fp = fopen(output_file, "w");
        if (fp == NULL) {
            perror("fopen");
            unmountfs();
            return -1;
        }

        for (int j = 0; j < OP_ITERATIONS; j++) {
            sprintf(target, "%s/%s%d", MOUNT_POINT, file_name, j);

            gettimeofday(&tv_start, NULL);
            ret = mkdir(target, 0777);
            gettimeofday(&tv_end, NULL);
            if (ret < 0) {
                perror("mkdir");
                unmountfs();
                return ret;
            }

            // microseconds
            latency = calculate_latency(tv_start, tv_end);
            fprintf(fp, "%ld,\n", latency);
        }

        fclose(fp);

        sleep(1);
        
        ret = unmountfs();
        if (ret < 0) {
            return ret;
        }  
    }
    return 0;
}

int measure_creat(char *mount_type, char *output_path) {
    int ret;
    FILE* fp;
    char *file_name = "file";
    char target[64];
    char output_file[NAME_LEN];
    struct timeval tv_start, tv_end;
    unsigned long latency;

    printf("Measuring creat latency\n");

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        ret = mountfs(mount_type);
        if (ret < 0) {
            return ret;
        }
        sprintf(output_file, "%s/creat/Run%d", output_path, i);
        fp = fopen(output_file, "w");
        if (fp == NULL) {
            perror("fopen");
            unmountfs();
            return -1;
        }

        for (int j = 0; j < OP_ITERATIONS; j++) {
            sprintf(target, "%s/%s%d", MOUNT_POINT, file_name, j);
            
            gettimeofday(&tv_start, NULL);
            ret = open(target, O_CREAT | O_RDWR);
            gettimeofday(&tv_end, NULL);
            if (ret < 0) {
                perror("creat");
                unmountfs();
                return ret;
            }

            // microseconds
            latency = calculate_latency(tv_start, tv_end);
            fprintf(fp, "%ld,\n", latency);
            close(ret);
        }

        fclose(fp);

        ret = unmountfs();
        if (ret < 0) {
            return ret;
        }  
    }
    return 0;
}

int measure_append_1k(char *mount_type, char *output_path) {
    int ret, fd;
    FILE* fp;
    char *file_name = "file";
    char target[64];
    char output_file[NAME_LEN];
    struct timeval tv_start, tv_end;
    unsigned long latency;
    char* write_buffer = malloc(SMALL_BUFFER_SIZE);
    memset(write_buffer, 'a', SMALL_BUFFER_SIZE);

    printf("Measuring 1k append latency\n");

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        ret = mountfs(mount_type);
        if (ret < 0) {
            return ret;
        }
        sprintf(output_file, "%s/append_1k/Run%d", output_path, i);
        fp = fopen(output_file, "w");
        if (fp == NULL) {
            perror("fopen");
            unmountfs();
            return -1;
        }

        for (int j = 0; j < OP_ITERATIONS; j++) {
            sprintf(target, "%s/%s%d", MOUNT_POINT, file_name, j);
            
            fd = open(target, O_CREAT | O_RDWR);
            
            if (ret < 0) {
                perror("creat");
                unmountfs();
                return ret;
            }


            gettimeofday(&tv_start, NULL);
            ret = write(fd, write_buffer, SMALL_BUFFER_SIZE);
            gettimeofday(&tv_end, NULL);
            if (ret < 0) {
                perror("write");
                unmountfs();
                return ret;
            }
            // microseconds
            latency = calculate_latency(tv_start, tv_end);
            fprintf(fp, "%ld,\n", latency);

            close(fd);
        }

        fclose(fp);

        ret = unmountfs();
        if (ret < 0) {
            return ret;
        }  
    }
    return 0;
}

int measure_append_16k(char *mount_type, char *output_path) {
    int ret, fd;
    FILE* fp;
    char *file_name = "file";
    char target[64];
    char output_file[NAME_LEN];
    struct timeval tv_start, tv_end;
    unsigned long latency;
    char* write_buffer = malloc(MED_BUFFER_SIZE);
    memset(write_buffer, 'a', MED_BUFFER_SIZE);

    printf("Measuring 16k append latency\n");

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        ret = mountfs(mount_type);
        if (ret < 0) {
            return ret;
        }
        sprintf(output_file, "%s/append_16k/Run%d", output_path, i);
        fp = fopen(output_file, "w");
        if (fp == NULL) {
            perror("fopen");
            unmountfs();
            return -1;
        }

        for (int j = 0; j < OP_ITERATIONS; j++) {
            sprintf(target, "%s/%s%d", MOUNT_POINT, file_name, j);
            
            fd = open(target, O_CREAT | O_RDWR);
            
            if (ret < 0) {
                perror("creat");
                unmountfs();
                return ret;
            }


            gettimeofday(&tv_start, NULL);
            ret = write(fd, write_buffer, MED_BUFFER_SIZE);
            gettimeofday(&tv_end, NULL);
            if (ret < 0) {
                perror("write");
                unmountfs();
                return ret;
            }
            // microseconds
            latency = calculate_latency(tv_start, tv_end);
            fprintf(fp, "%ld,\n", latency);

            close(fd);
        }

        fclose(fp);

        ret = unmountfs();
        if (ret < 0) {
            return ret;
        }  
    }
    return 0;
}

int measure_append_64k(char *mount_type, char *output_path) {
    int ret, fd;
    FILE* fp;
    char *file_name = "file";
    char target[64];
    char output_file[NAME_LEN];
    struct timeval tv_start, tv_end;
    unsigned long latency;
    char* write_buffer = malloc(BIG_BUFFER_SIZE);
    memset(write_buffer, 'a', BIG_BUFFER_SIZE);

    printf("Measuring 64k append latency\n");

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        ret = mountfs(mount_type);
        if (ret < 0) {
            return ret;
        }
        sprintf(output_file, "%s/append_64k/Run%d", output_path, i);
        fp = fopen(output_file, "w");
        if (fp == NULL) {
            perror("fopen");
            unmountfs();
            return -1;
        }

        for (int j = 0; j < OP_ITERATIONS; j++) {
            sprintf(target, "%s/%s%d", MOUNT_POINT, file_name, j);
            
            fd = open(target, O_CREAT | O_RDWR);
            
            if (ret < 0) {
                perror("creat");
                unmountfs();
                return ret;
            }


            gettimeofday(&tv_start, NULL);
            ret = write(fd, write_buffer, BIG_BUFFER_SIZE);
            gettimeofday(&tv_end, NULL);
            if (ret < 0) {
                perror("write");
                unmountfs();
                return ret;
            }
            // microseconds
            latency = calculate_latency(tv_start, tv_end);
            fprintf(fp, "%ld,\n", latency);

            close(fd);
        }

        fclose(fp);

        ret = unmountfs();
        if (ret < 0) {
            return ret;
        }  
    }
    return 0;
}


int measure_read_1k(char *mount_type, char *output_path) {
    int ret, fd;
    FILE* fp;
    char *file_name = "file";
    char target[64];
    char output_file[NAME_LEN];
    struct timeval tv_start, tv_end;
    unsigned long latency;
    char* buffer = malloc(SMALL_BUFFER_SIZE);
    memset(buffer, 'a', SMALL_BUFFER_SIZE);

    printf("Measuring 1k append latency\n");

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        ret = mountfs(mount_type);
        if (ret < 0) {
            return ret;
        }
        sprintf(output_file, "%s/read_1k/Run%d", output_path, i);
        fp = fopen(output_file, "w");
        if (fp == NULL) {
            perror("fopen");
            unmountfs();
            return -1;
        }

        // fill in the file
        for (int j = 0; j < OP_ITERATIONS; j++) {
            sprintf(target, "%s/%s%d", MOUNT_POINT, file_name, j);
            
            fd = open(target, O_CREAT | O_RDWR);
            if (ret < 0) {
                perror("creat");
                unmountfs();
                return ret;
            }

            ret = write(fd, buffer, SMALL_BUFFER_SIZE);
            if (ret < 0) {
                perror("write");
                unmountfs();
                return ret;
            }

            close(fd);
        }

        for (int j = 0; j < OP_ITERATIONS; j++) {
            sprintf(target, "%s/%s%d", MOUNT_POINT, file_name, j);
            
            fd = open(target, O_CREAT | O_RDWR);
            
            if (ret < 0) {
                perror("creat");
                unmountfs();
                return ret;
            }


            gettimeofday(&tv_start, NULL);
            ret = read(fd, buffer, SMALL_BUFFER_SIZE);
            gettimeofday(&tv_end, NULL);
            if (ret < 0) {
                perror("write");
                unmountfs();
                return ret;
            }
            // microseconds
            latency = calculate_latency(tv_start, tv_end);
            fprintf(fp, "%ld,\n", latency);

            close(fd);
        }

        fclose(fp);

        ret = unmountfs();
        if (ret < 0) {
            return ret;
        }  
    }
    return 0;
}

int measure_read_16k(char *mount_type, char *output_path) {
    int ret, fd;
    FILE* fp;
    char *file_name = "file";
    char target[64];
    char output_file[NAME_LEN];
    struct timeval tv_start, tv_end;
    unsigned long latency;
    char* buffer = malloc(MED_BUFFER_SIZE);
    memset(buffer, 'a', MED_BUFFER_SIZE);

    printf("Measuring 16k read latency\n");

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        ret = mountfs(mount_type);
        if (ret < 0) {
            return ret;
        }
        sprintf(output_file, "%s/read_16k/Run%d", output_path, i);
        fp = fopen(output_file, "w");
        if (fp == NULL) {
            perror("fopen");
            unmountfs();
            return -1;
        }

        // fill in the file
        for (int j = 0; j < OP_ITERATIONS; j++) {
            sprintf(target, "%s/%s%d", MOUNT_POINT, file_name, j);
            
            fd = open(target, O_CREAT | O_RDWR);
            if (ret < 0) {
                perror("creat");
                unmountfs();
                return ret;
            }

            ret = write(fd, buffer, MED_BUFFER_SIZE);
            if (ret < 0) {
                perror("write");
                unmountfs();
                return ret;
            }

            close(fd);
        }

        for (int j = 0; j < OP_ITERATIONS; j++) {
            sprintf(target, "%s/%s%d", MOUNT_POINT, file_name, j);
            
            fd = open(target, O_CREAT | O_RDWR);
            
            if (ret < 0) {
                perror("creat");
                unmountfs();
                return ret;
            }


            gettimeofday(&tv_start, NULL);
            ret = read(fd, buffer, MED_BUFFER_SIZE);
            gettimeofday(&tv_end, NULL);
            if (ret < 0) {
                perror("write");
                unmountfs();
                return ret;
            }
            // microseconds
            latency = calculate_latency(tv_start, tv_end);
            fprintf(fp, "%ld,\n", latency);

            close(fd);
        }

        fclose(fp);

        ret = unmountfs();
        if (ret < 0) {
            return ret;
        }  
    }
    return 0;
}

int measure_read_64k(char *mount_type, char *output_path) {
    int ret, fd;
    FILE* fp;
    char *file_name = "file";
    char target[64];
    char output_file[NAME_LEN];
    struct timeval tv_start, tv_end;
    unsigned long latency;
    char* buffer = malloc(BIG_BUFFER_SIZE);
    memset(buffer, 'a', BIG_BUFFER_SIZE);

    printf("Measuring 64k read latency\n");

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        ret = mountfs(mount_type);
        if (ret < 0) {
            return ret;
        }
        sprintf(output_file, "%s/read_64k/Run%d", output_path, i);
        fp = fopen(output_file, "w");
        if (fp == NULL) {
            perror("fopen");
            unmountfs();
            return -1;
        }

        // fill in the file
        for (int j = 0; j < OP_ITERATIONS; j++) {
            sprintf(target, "%s/%s%d", MOUNT_POINT, file_name, j);
            
            fd = open(target, O_CREAT | O_RDWR);
            if (ret < 0) {
                perror("creat");
                unmountfs();
                return ret;
            }

            ret = write(fd, buffer, MED_BUFFER_SIZE);
            if (ret < 0) {
                perror("write");
                unmountfs();
                return ret;
            }

            close(fd);
        }
        

        for (int j = 0; j < OP_ITERATIONS; j++) {
            sprintf(target, "%s/%s%d", MOUNT_POINT, file_name, j);
            
            fd = open(target, O_CREAT | O_RDWR);
            
            if (ret < 0) {
                perror("creat");
                unmountfs();
                return ret;
            }


            gettimeofday(&tv_start, NULL);
            ret = read(fd, buffer, BIG_BUFFER_SIZE);
            gettimeofday(&tv_end, NULL);
            if (ret < 0) {
                perror("write");
                unmountfs();
                return ret;
            }
            // microseconds
            latency = calculate_latency(tv_start, tv_end);
            fprintf(fp, "%ld,\n", latency);

            close(fd);
        }

        fclose(fp);

        ret = unmountfs();
        if (ret < 0) {
            return ret;
        }  
    }
    return 0;
}

int measure_unlink(char *mount_type, char *output_path) {
    int ret, fd;
    FILE* fp;
    char *file_name = "file";
    char target[64];
    char output_file[NAME_LEN];
    struct timeval tv_start, tv_end;
    unsigned long latency;
    char* write_buffer = malloc(MED_BUFFER_SIZE);
    memset(write_buffer, 'a', MED_BUFFER_SIZE);

    printf("Measuring unlink latency\n");

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        ret = mountfs(mount_type);
        if (ret < 0) {
            return ret;
        }
        sprintf(output_file, "%s/unlink/Run%d", output_path, i);
        fp = fopen(output_file, "w");
        if (fp == NULL) {
            perror("fopen");
            unmountfs();
            return -1;
        }

        // first create and write to all of the files
        for (int j = 0; j < OP_ITERATIONS; j++) {
            sprintf(target, "%s/%s%d", MOUNT_POINT, file_name, j);
            
            fd = open(target, O_CREAT | O_RDWR);
            
            if (ret < 0) {
                perror("creat");
                unmountfs();
                return ret;
            }

            ret = write(fd, write_buffer, MED_BUFFER_SIZE);
            if (ret < 0) {
                perror("write");
                unmountfs();
                return ret;
            }

            close(fd);
        }

        for (int j = 0; j < OP_ITERATIONS; j++) {
            sprintf(target, "%s/%s%d", MOUNT_POINT, file_name, j);
            
            gettimeofday(&tv_start, NULL);
            ret = unlink(target);
            gettimeofday(&tv_end, NULL);
            if (ret < 0) {
                perror("unlink");
                unmountfs();
                return ret;
            }

            // microseconds
            latency = calculate_latency(tv_start, tv_end);
            fprintf(fp, "%ld,\n", latency);
        }

        fclose(fp);

        ret = unmountfs();
        if (ret < 0) {
            return ret;
        }  
    }
    return 0;
}

int measure_rename(char *mount_type, char *output_path) {
    int ret;
    FILE* fp;
    char *file_name = "dir";
    char target[64], source[64];
    char output_file[NAME_LEN];
    struct timeval tv_start, tv_end;
    unsigned long latency;

    printf("Measuring rename latency\n");

    for (int i = 0; i < TEST_ITERATIONS; i++) {
        ret = mountfs(mount_type);
        if (ret < 0) {
            return ret;
        }
        sprintf(output_file, "%s/rename/Run%d", output_path, i);
        fp = fopen(output_file, "w");
        if (fp == NULL) {
            perror("fopen");
            unmountfs();
            return -1;
        }

        sprintf(target, "%s/src_parent/", MOUNT_POINT);
        ret = mkdir(target, 0777);
        if (ret < 0) {
            perror("mkdir");
            unmountfs();
            return ret;
        }
        sprintf(target, "%s/dst_parent/", MOUNT_POINT);
        ret = mkdir(target, 0777);
        if (ret < 0) {
            perror("mkdir");
            unmountfs();
            return ret;
        }

        for (int j = 0; j < OP_ITERATIONS; j++) {
            sprintf(target, "%s/src_parent/%s%d", MOUNT_POINT, file_name, j);
            gettimeofday(&tv_start, NULL);
            ret = mkdir(target, 0777);
            gettimeofday(&tv_end, NULL);
            if (ret < 0) {
                perror("mkdir");
                unmountfs();
                return ret;
            }

            // microseconds
            latency = calculate_latency(tv_start, tv_end);
            // fprintf(fp, "%ld,\n", latency);
        }

        sleep(4);

        for (int j = 0; j < OP_ITERATIONS; j++) {
            sprintf(source, "%s/src_parent/%s%d", MOUNT_POINT, file_name, j);
            sprintf(target, "%s/dst_parent/%s%d", MOUNT_POINT, file_name, j);

            gettimeofday(&tv_start, NULL);
            ret = rename(source, target);
            gettimeofday(&tv_end, NULL);
            if (ret < 0) {
                perror("rename");
                unmountfs();
                return ret;
            }

            // microseconds
            latency = calculate_latency(tv_start, tv_end);
            fprintf(fp, "%ld,\n", latency);
        }

        fclose(fp);
        
        ret = unmountfs();
        if (ret < 0) {
            return ret;
        }  
    }
    return 0;
}

int main(int argc, void *argv[]) {
    int ret;
    char *fs_type;
    char *mount_type;
    unsigned long avg_mkdir_latency;
    char output_path[NAME_LEN];
    memset(output_path, 0, NAME_LEN);

    if (argc < 2) {
        printf("Usage: syscall_latency filesystem\n");
        return 1;
    }
    fs_type = mount_type = (char*)argv[1];

    if (strcmp(fs_type, "squirrelfs") == 0) {
        mount_type = "hayleyfs";
    } else if (strcmp(fs_type, "nova") == 0) {
        mount_type = "NOVA";
    }

    sprintf(output_path, "%s/%s/syscall_latency", OUTPUT_DIR, fs_type);

    ret = measure_mkdir(mount_type, output_path);
    if (ret < 0) {
        return ret;
    }

    ret = measure_creat(mount_type, output_path);
    if (ret < 0) {
        return ret;
    }

    ret = measure_append_1k(mount_type, output_path);
    if (ret < 0) {
        return ret;
    }

    ret = measure_append_16k(mount_type, output_path);
    if (ret < 0) {
        return ret;
    }

    // ret = measure_append_64k(mount_type, output_path);
    // if (ret < 0) {
    //     return ret;
    // }

    ret = measure_read_1k(mount_type, output_path);
    if (ret < 0) {
        return ret;
    }

    ret = measure_read_16k(mount_type, output_path);
    if (ret < 0) {
        return ret;
    }

    // ret = measure_read_64k(mount_type, output_path);
    // if (ret < 0) {
    //     return ret;
    // }

    ret = measure_unlink(mount_type, output_path);
    if (ret < 0) {
        return ret;
    }

    ret = measure_rename(mount_type, output_path);
    if (ret < 0) {
        return ret;
    }

    return 0;
}