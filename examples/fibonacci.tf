# --- fibonacci.tf ---
# Defining "fib" function
[ 

#N
  
  # 1. verify N <= 1

  dup 1 <= 
  
  # 2. Fake branch : Recursive Call
  [ 
    # Lo stack attuale ha solo: n
    
    dup 1 - 
    fib 
     
    # We need (n-2) so we need to swap the stacl
    swap 2 -    # Stack: fib(n-1) (n-2)
    
    fib         # Chiamata ricorsiva. Stack: fib(n-1) fib(n-2)
    +           # Somma i due risultati. Stack: [risultato finale]
  ] 
  
  # Truth case (n <= 1)
  [
   # Empty list are supported.
  ] 
  
  # If construct
  if 
  
] "fib" def


# --- Test area ---

# Calculate 10° fibonacci number (0, 1, 1, 2, 3, 5, 8)
# Expected result is 55.

10 fib 

print
