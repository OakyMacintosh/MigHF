import typer

app = typer.Typer()
# command = 0

@app.command()
def repl():
    def print_help():
        print("Commands:")
        print("    ,help")
        print("    ,quit")
        print("")

    while True:
        command = input("(mhf): ")
       # print(f"(mhf): {command}")
        if command == ",help":
            print_help()
        elif command == ",quit":
            exit(0)
        else:
            print(f"Unexpected command")

@app.command()
def main():
    pass

if __name__ == "__main__":
    app()
