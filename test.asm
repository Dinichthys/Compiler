push 1
pop bx

push bx
                out

push 0
pop cx

push 5
pop [cx+0]

push [cx+0]
                out

push [cx+0]
pop [bx+2]

push [bx+2]
                out

push 1
pop [bx+3]

push [bx+2]
                out

push [bx+3]
push [bx+2]
eq

                out

hlt
