#include <stdio.h>
#include <string.h>

int pattern2(const char *string)
{
  int length = strlen(string);
  if (length < 3)
  {
    return 0;
  }
  if (string[length - 1] != 'b' || string[length - 2] != 'b')
  {
    return 0;
  }
  for (int i = 0; i < length - 2; i++)
  {
    if (string[i] != 'a')
    {
      return 0;
    }
  }

  return 1;
}

int main()
{
  char string[100];
  int test;

  printf("Enter the number of test cases: ");
  scanf("%d", &test);

  for (int i = 0; i < test; i++)
  {
    printf("Enter string for test case %d: ", i + 1);
    scanf("%s", string);

    if (pattern2(string))
    {
      printf("Valid string: %s\n", string);
    }
    else
    {
      printf("Invalid string: %s\n", string);
    }
  }

  return 0;
}
