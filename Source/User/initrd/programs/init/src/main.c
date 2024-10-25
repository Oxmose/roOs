static int get_errno();
char myValue = 4;
volatile char otherValues[] = "This is a strign yes";
char myTask[50] = {0};


__thread int errno;
static int get_errno() { return errno; }

int main(void)
{
    unsigned int i;
    i = 0;
    while(1)
    {
        myTask[i % 50] = myValue;
        ++i;
        myValue = get_errno();
        (void)otherValues;
    }

    return 0;
}