program MACMIGHF
uses
  MacOSAll;

{$R *.RES}

begin
  // Initialize the Macintosh system
  InitGraf(@thePort);
  InitFonts;
  InitWindows;
  InitMenus;
  TEInit;
  
  // Your application code goes here
  
  // Clean up and exit
  TEExit;
end.

{
  This program initializes the Classic Mac OS 9 environment.
  It sets up the graphics, fonts, windows, and menus.
  You can add your application logic where indicated.
}