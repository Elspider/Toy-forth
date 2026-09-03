
[ 
  dup 1 = 
  [ dup 1 - factorial * ]  # False branch: n * factorial(n-1)
  [ 1 ]                    # True branch: return 1
  if 
] "factorial" def


# example
10 factorial print
