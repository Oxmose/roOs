char myValue = 4;
char myTask[50] = {0};

int main(void)
{
    unsigned int i;
    i = 0;
    while(1)
    {
        myTask[i % 50] = myValue;
        ++i;
    }

    return 0;
}