#include <stdio.h>      /* printf   */
#include <ctype.h>      /* isalpha isdigit isspace etc      */

#define FALSE 0
#define TRUE  1

/* function declarations */
int char_type(char);

main()
{
 char ch;

 ch = 127;
 char_type(ch);

 ch = '\b';
 char_type(ch);

 return 0;
}

int char_type(char ch)
{
 if ( iscntrl(ch) != FALSE)
   printf("%c is a control character\n", ch); 
}
