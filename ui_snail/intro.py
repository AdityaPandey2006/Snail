import tkinter as tk
from tkinter import scrolledtext
import subprocess
import threading
import os
import pty
import subprocess

#we are using basically a pty a pseudo terminal that tricks shell to think that we are running in a real terminal


snail_art=(
"                                                              d::\n"
"                                                              ''d$:     :h\n"
"                                                                 d$   :$h'\n"
"                              .......                             d$..$h'\n"
"                         ..cc$$$$$$$$$c.                  ...c$$$$$$$$$h\n"
"                       .d$$'        '$$$h.              cc$$$$:::::d$!$h\n"
"                     c$$'              '$$c           .$$$$:::()::d$!$h'\n"
"                   .c$$'                 $$h.        .d$$::::::::d$!!$h\n"
"                 .$$h'                    $$$.       $$$::::::::$$!!$h'\n"
"                .$$h'      .;dd,           $$$.      $$$:::::::d$!!$h'\n"
"                $$$'      dd$$$hh,          $$$.     $$$:::::::d$!!$h\n"
"                $$$      d$$' '$hh.          $$$.   .$$$:::::d$!!!!$h\n"
"                $$$      $$d    $$$           d$$.  $$$:::::d$!!!!$h'\n"
"                $$$      $$$ d  $$$          .d$$$.c$$::::::d$!!!!$h\n"
"                $$$.     '$$$$  $$$         .$$$$$$$$::::::d$$!!!$h'\n"
"                '$$h.           $$$       .d$$:::::::::::::d$$!!!$h\n"
"                  \"$\"$$h        .$$$    ,$$$:::::::::::::::d$$!!!$h'\n"
"..hh              .c$$:$h       $$$'   ,$$$::::::::::::::d$$!!!$h'\n"
"d$$           .c$$$:::$$h,     $$h    $$$:::::::::::::d$$$!!!$h'\n"
"$$$.        .$$h::::::::$$$dddd$h'   $$$::::::::::::dd$$!!!!$h'\n"
"'$$$$c...cc$$h:::::::::::$$$$$$$hhhh$$$::::::::::d$$$$$$$hhh'\n"
" '$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$$hhhhhhhh'''\n"
)

master_fd, slave_fd = pty.openpty()  

shell = subprocess.Popen(
    ["../snail"],
    stdin=slave_fd,
    stdout=slave_fd,
    stderr=slave_fd,
    #cwd="..", as this file runs in ui_snail added this to retuern to parent directory of snail this does not wotjin ig mounted systems
    text=True,
    close_fds=True
)

#UI setup
root = tk.Tk()   #basic needed to setup tkinter 
root.title("Snail Shell UI(baisic for dev)")
root.geometry("1300x700")

main_frame = tk.Frame(root)
main_frame.pack(fill="both", expand=True)

#contains the snail symbol
left_frame = tk.Frame(main_frame, width=250, bg="#111111")
left_frame.pack(side="left", fill="y")

#the actual prompt accepting 
right_frame = tk.Frame(main_frame,bg="#111111")
right_frame.pack(side="right", fill="both", expand=True)

snail_frame=tk.Frame(left_frame, height=200, bg="#111111")
snail_frame.pack(side="top", fill="x")

bottom_frame = tk.Frame(left_frame, bg="#1a1a1a")
bottom_frame.pack(side="top", fill="both", expand=True) #in this bottom frame we will add something unique...


snail_box = tk.Text(
    snail_frame,
    bg="#111111",
    fg="#00ff9c",
    font=("Consolas", 8),
    bd=0,
    highlightthickness=0
)
snail_box.pack(fill="both", expand=True)

snail_box.insert("1.0", snail_art)
snail_box.config(state="disabled")

output_box=scrolledtext.ScrolledText(
    right_frame,
    bg="#1e1e1e",  #black bacground
    fg="#d4d4d4",  #white text colour
    insertbackground="white",
    font=("Consolas", 12)
)
output_box.pack(fill="both", expand=True)

#tag fro prompt styling
output_box.tag_config("prompt", foreground="#00ff9c")# green prompt

entry = tk.Entry(
    right_frame,
    bg="#2d2d2d",
    fg="white",
    insertbackground="white",
    font=("Consolas", 12)
)
entry.pack(fill="x")

#Read (normal output)

def read_output():
    while True:
        data = os.read(master_fd, 1024).decode()
        if data:
            output_box.insert(tk.END, data)
            output_box.see(tk.END)

# Send command
def send_command(event=None):
    cmd = entry.get() + "\n"
    os.write(master_fd, cmd.encode())
    entry.delete(0, tk.END)

entry.bind("<Return>", send_command) #pressing enter sends the commandn 

#Threading to perform multiple tasks concurrently within the same process.
#needed to add threading to basically haev inputs entered and output read in dynamically from the bacground pseudp terminal  in which the shell is actually running it without freexing the gui
threading.Thread(target=read_output, daemon=True).start()

root.mainloop()