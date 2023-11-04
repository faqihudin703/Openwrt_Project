import time
start_time = time.time()

print("Hello")
print("World")

print("Say Hello")
# ini adalah comment
a = 10 # ini adalah comment
"""Hello Friend
How are you"""
print(a)
for i in range(1,100000):
    a = 100
    
print(time.time() - start_time, "detik")
# kita bisa mengcompile python ke
# yang namanya bytecode
# cara mengcompile, buka terinal & ketik
# python -m py_compile main.py
# cd __pycache__   python main.cpython-311.pyc
# python __pycache__/main.cpython-311.pyc
