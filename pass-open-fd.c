/**
 * Pass an open file descriptor between two processes.
 *
 * Adapted from code found here:
 * https://stackoverflow.com/questions/28003921/sending-file-descriptor-by-linux-socket/
 *
 * N. P. O'Donnell, 2021.
 */

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include <unistd.h>

static
void wyslij(int socket, int fd)  // send fd by socket
{
    struct msghdr msg = { 0 };
    char buf[CMSG_SPACE(sizeof(fd))];
    memset(buf, '\0', sizeof(buf));
    struct iovec io = { .iov_base = "ABC", .iov_len = 3 };
    
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    msg.msg_control = buf;
    msg.msg_controllen = sizeof(buf);
    
    struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg);
    cmsg->cmsg_level = SOL_SOCKET;
    cmsg->cmsg_type = SCM_RIGHTS;
    cmsg->cmsg_len = CMSG_LEN(sizeof(fd));
    
    *((int *) CMSG_DATA(cmsg)) = fd;
    
    msg.msg_controllen = CMSG_SPACE(sizeof(fd));
    
    if (sendmsg(socket, &msg, 0) < 0) {
        perror("sendmsg");
    }
    
}

static
int odbierz(int socket)  // receive fd from socket
{
    struct msghdr msg = {0};
    
    char m_buffer[256];
    struct iovec io = { .iov_base = m_buffer, .iov_len = sizeof(m_buffer) };
    msg.msg_iov = &io;
    msg.msg_iovlen = 1;
    
    char c_buffer[256];
    msg.msg_control = c_buffer;
    msg.msg_controllen = sizeof(c_buffer);
    
    if (recvmsg(socket, &msg, 0) < 0) {
        perror("recvmsg");
    }
    
    struct cmsghdr * cmsg = CMSG_FIRSTHDR(&msg);
    
    unsigned char * data = CMSG_DATA(cmsg);
    
    printf("About to extract fd\n");
    int fd = *((int*) data);
    printf("Extracted fd %d\n", fd);
    
    return fd;
}

int main(int argc, char **argv)
{
    char* filename;
    if (argc > 1) {
        filename = argv[1];
    } else {
        printf("Usage: %s <filename>\n", argv[0]);
        return 1;
    }
    
    int sv[2];
    if (socketpair(AF_UNIX, SOCK_DGRAM, 0, sv) != 0) {
        perror("socketpair");
    }
    
    int pid = fork();
    if (pid > 0)  // in parent
    {
        printf("Parent at work\n");
        close(sv[1]);
        int sock = sv[0];
        
        int fd = open(filename, O_RDONLY);
        if (fd < 0) {
            perror("open");
            fprintf(stderr, "Failed to open file %s for reading\n", filename);
        }
        
        wyslij(sock, fd);
        
        close(fd);
        nanosleep(&(struct timespec){ .tv_sec = 1, .tv_nsec = 500000000}, 0);
        printf("Parent exits\n");
    }
    else  // in child
    {
        printf("Child at play\n");
        close(sv[0]);
        int sock = sv[1];
        
        nanosleep(&(struct timespec){ .tv_sec = 0, .tv_nsec = 500000000}, 0);
        
        int fd = odbierz(sock);
        printf("Read %d!\n", fd);
        char buffer[256];
        ssize_t nbytes;
        while ((nbytes = read(fd, buffer, sizeof(buffer))) > 0)
            write(1, buffer, nbytes);
        printf("Done!\n");
        close(fd);
    }
    return 0;
}
