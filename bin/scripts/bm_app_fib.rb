def fib n
  if n < 3
    1
  else
    fib(n-1) + fib(n-2)
  end
end
#LLRB::JIT.compile(self, :fib)

fib(34)
