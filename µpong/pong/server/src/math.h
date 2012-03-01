#ifndef PONG_MATH_H__
#define PONG_MATH_H__

int sin( int x );
int cos( int x );

inline int sign( int x ) 
{ 
  if (x<0) 
    return -1; 
  else 
    return 1;
}

inline int abs( int x )
{
  if (x<0) 
    return -x;
  else
    return x;
}

#endif // PONG_MATH_H__

