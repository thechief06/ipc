#include <sys/types.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <pthread.h>

const char *p_fifopath = "producerMessage.txt"; ///home/gokhan/ipcCommunication/
const char *c_fifopath = "consumerMessage.txt";
int pipefd[2];

void *pipeProducer(void *);
void *pipeConsumer(void *);

void *namedPipeProducer(void *);
void *namedPipeConsumer(void *);

void *sharedMemoryProducer(void *);
void *sharedMemoryConsumer(void *);

void *messageQueueProducer(void *);
void *messageQueueConsumer(void *);

int written = 0;

// structure for message queue
struct mesg_buffer
{
    long mesg_type;
    char mesg_text[5];

} message;
char output[5] = "pi";
char input;

void start_shell()
{
    printf("\n--------------------------------------------------------------\n");
    printf("|                              IPC                              |\n");
    printf("|                      developed by gyurtalan                   |\n");
    printf("--------------------------------------------------------------\n");
}

void help()
{
    printf("-----------------------------------------\n");
    printf("| Enter                                 |\n");
    printf("| p to Pipe mechanizm                   |\n");
    printf("| n to NamedPipe mechanizm              |\n");
    printf("| m to MessageQueue mechanizm           |\n");
    printf("| s to SharedMemory mechanizm           |\n");
    printf("| q to exit                             |\n");
    printf("-----------------------------------------\n");
}

void *pipeProducer(void *vargp)
{
    written = 0;
    FILE *file = fopen(p_fifopath, "w");

    close(pipefd[0]);
    for (int i = 0; i < 65; i++)
    {
        if (i + 1 < 10)
        {
            output[2] = '0';
            output[3] = i + 1 + '0';
        }
        else
        {
            output[2] = ((i + 1) / 10) + '0';
            output[3] = ((i + 1) % 10) + '0';
        }

        write(pipefd[1], &output, sizeof(int));
        fprintf(file, "%s\n", output);
        printf("producer: %s\n", output);
    }
    fclose(file);
    close(pipefd[1]);
    wait(NULL);
    written = 1;

    return 0;
}

void *pipeConsumer(void *vargp)
{
    while (1)
    {
        if (written)
        {
            FILE *file = fopen(c_fifopath, "w");

            close(pipefd[1]);
            while (read(pipefd[0], &input, sizeof(int)) != 0)
            {
                fprintf(file, "%c\n", input);
                printf("consumer : %c\n", input);
            }
            fclose(file);
            close(pipefd[0]);
            written = 0;
            return;
        }
    }
    return 0;
}

void *namedPipeProducer(void *vargp)
{

    mkfifo(p_fifopath, 0666);
    written = 0;

    for (int i = 0; i < 65; i++)
    {
        if (i + 1 < 10)
        {
            output[2] = '0';
            output[3] = i + 1 + '0';
        }
        else
        {
            output[2] = ((i + 1) / 10) + '0';
            output[3] = ((i + 1) % 10) + '0';
        }
        // Open FIFO for write only
        pipefd[0] = open(p_fifopath, O_WRONLY);

        // Write the input arr2ing on FIFO
        // and close it
        write(pipefd[0], output, strlen(output) + 1);
        close(pipefd[0]);
        printf("producer: %s\n", output);
    }
    written = 1;

    return 0;
}
void *namedPipeConsumer(void *vargp)
{
    // Creating the named file(FIFO)
    // mkfifo(<pathname>,<permission>)

    mkfifo(p_fifopath, 0666);
    while (1)
    {
        if (written == 1)
        {
            // First open in read only and read
            pipefd[1] = open(p_fifopath, O_RDONLY);
            read(pipefd[1], input, 64);

            // Print the read string and close
            printf("consumer : %d\n", input);
            close(pipefd[1]);
            written = 0;
            return;
        }
    }
    return 0;
}

void *sharedMemoryProducer(void *vargp)
{
    written = 0;
    // ftok to generate unique key
    key_t key = ftok("shmfile", 65);

    // shmget returns an identifier in shmid
    int shmid = shmget(key, 1024, 0666 | IPC_CREAT);

    // shmat to attach to shared memory
    char *str = (char *)shmat(shmid, (void *)0, 0);

    for (int i = 0; i < 65; i++)
    {

        if (i + 1 < 10)
        {
            output[2] = '0';
            output[3] = i + 1 + '0';
        }
        else
        {
            output[2] = ((i + 1) / 10) + '0';
            output[3] = ((i + 1) % 10) + '0';
        }

        //detach from shared memory
        shmdt(output);

        // display the message
        printf("Data send is : %s \n", output);
    }
    written = 1;

    return 0;
}

void *sharedMemoryConsumer(void *vargp)
{
    while (1)
    {
        if (written == 1)
        {
            // ftok to generate unique key
            key_t key = ftok("shmfile", 65);

            // shmget returns an identifier in shmid
            int shmid = shmget(key, 1024, 0666 | IPC_CREAT);

            // shmat to attach to shared memory
            char *str = (char *)shmat(shmid, (void *)0, 0);

            printf("Data read from memory: %s\n", str);

            //detach from shared memory
            shmdt(str);

            // destroy the shared memory
            shmctl(shmid, IPC_RMID, NULL);

            written = 0;
            return;
        }
    }
    return 0;
}

void *messageQueueProducer(void *vargp)
{
    key_t key;
    int msgid;
    written = 0;

    if (written == 0)
    {
        // ftok to generate unique key
        key = ftok("progfile", 65);

        // msgget creates a message queue
        // and returns identifier
        msgid = msgget(key, 0666 | IPC_CREAT);

        message.mesg_type = 1;
        memcpy(message.mesg_text, "pi", 5);

        for (int i = 0; i < 65; i++)
        {

            if (i + 1 < 10)
            {
                message.mesg_text[2] = '0';
                message.mesg_text[3] = i + 1 + '0';
            }
            else
            {
                message.mesg_text[2] = ((i + 1) / 10) + '0';
                message.mesg_text[3] = ((i + 1) % 10) + '0';
            }

            // msgsnd to send message
            msgsnd(msgid, &message, sizeof(message), 0);

            // display the message
            printf("Data send is : %s \n", message.mesg_text);
        }
        written = 1;
    }
    return 0;
}
void *messageQueueConsumer(void *vargp)
{
    while (1)
    {
        if (written == 1)
        {
            key_t key;
            int msgid;

            // ftok to generate unique key
            key = ftok("progfile", 65);

            // msgget creates a message queue
            // and returns identifier
            msgid = msgget(key, 0666 | IPC_CREAT);

            // msgrcv to receive message
            msgrcv(msgid, &message, sizeof(message), 1, 0);

            // display the message
            printf("Data Received is : %s \n",
                   message.mesg_text);

            // to destroy the message queue
            msgctl(msgid, IPC_RMID, NULL);
            written = 0;
            return;
        }
    }
    return 0;
}
int main()
{
    //declarations
    char ch;
    //starting the shell
    start_shell();
    //loop
    while (1)
    {

        help();
        scanf("%c", &ch);

        if (ch == 'p')
        {
            int pipefd[2];
            if (pipe(pipefd) == -1)
                return -1;

            pthread_t thread_id;

            pthread_create(&thread_id, NULL, pipeProducer, NULL);
            pthread_create(&thread_id, NULL, pipeConsumer, NULL);
            pthread_join(thread_id, NULL);
            printf("\nDone");
        }
        else if (ch == 'n')
        {
            int pipefd[2];
            if (pipe(pipefd) == -1)
                return -1;

            pthread_t thread_id;

            pthread_create(&thread_id, NULL, namedPipeProducer, NULL);
            pthread_create(&thread_id, NULL, namedPipeConsumer, NULL);
            pthread_join(thread_id, NULL);
            printf("\nDone");
        }
        else if (ch == 'm')
        {
            pthread_t thread_id;

            pthread_create(&thread_id, NULL, messageQueueProducer, NULL);
            pthread_create(&thread_id, NULL, messageQueueConsumer, NULL);
            pthread_join(thread_id, NULL);
            printf("\nDone");
        }
        else if (ch == 's')
        {
            int ch;
            pthread_t thread_id;

            pthread_create(&thread_id, NULL, sharedMemoryProducer, NULL);
            pthread_create(&thread_id, NULL, sharedMemoryConsumer, NULL);
            pthread_join(thread_id, NULL);
            printf("\nDone");
        }
        else if (ch == 'q')
        {
            return 0;
        }
    }
    return 0;
}
