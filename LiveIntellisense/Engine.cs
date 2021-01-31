using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Diagnostics;
using System.Text.RegularExpressions;

namespace LiveIntellisense
{
    class Engine
    {
        private ConsoleColor warningClr = ConsoleColor.DarkMagenta;
        private ConsoleColor errorClr = ConsoleColor.Red;

        public string cursorStr = "^|";

        private string inputDir;
        private int msTimeout;
        private string intellisenseProg;


        private string currentLocation;
        private DirectoryInfo di;

        // fullname x text
        private Dictionary<string, string> files;
        private string currentFile;

        public Engine(string inputDir, int msTimeout, string intellisenseProg)
        {
            if (!Directory.Exists(inputDir))
                throw new Exception(string.Format(
                    "Directory {0} does not exist!", inputDir));

            if (!File.Exists(intellisenseProg))
                throw new Exception(string.Format(
                    "Intellisense prog {0} not found!", intellisenseProg));

            this.inputDir = inputDir;
            this.msTimeout = msTimeout;
            this.intellisenseProg = intellisenseProg;

            currentLocation = System.IO.Path.GetDirectoryName(
                System.Reflection.Assembly.GetExecutingAssembly().Location);


            files = new Dictionary<string, string>();
            di = new DirectoryInfo(inputDir);
        }

        public void Run()
        {
            while (true)
            {
                bool updated = CheckForUpdates();

                if (updated)
                {
                    Printer.PrepareConsole("Running");
                    DoIntelliSense();
                }

                Thread.Sleep(msTimeout);
            }
        }

        private bool CheckForUpdates()
        {
            bool updated = false;
            foreach (var fi in di.GetFiles())
            {
                string text;

                // visible in dictionary?
                if (!files.ContainsKey(fi.FullName))
                {
                    updated = true; // added file
                    text = File.ReadAllText(fi.FullName);
                    files.Add(fi.FullName, text);
                }
                else
                {
                    text = File.ReadAllText(fi.FullName);
                    if (!text.Equals(files[fi.FullName]))
                    {
                        updated = true; // text changed
                        files[fi.FullName] = text;
                    }
                }
            }

            int cursorFound = 0;
            foreach (var f in files)
            {
                string text = f.Value;
                string filename = f.Key;

                // contains cursor?
                if (text.Contains(cursorStr))
                {
                    cursorFound++;
                    if (cursorFound > 1)
                    {
                        if (cursorFound == 2)
                        {
                            if (updated)
                            {
                                Printer.PrepareConsole("Error");
                                Printer.Print(string.Format(
                                    "More than one cursor occurance found.\nFirst at: {0}", currentFile), errorClr);
                            }
                        }

                        if (updated)
                            Printer.Print(string.Format("> Cursor also found at: {0}", filename), warningClr);
                    }
                    else
                    {
                        currentFile = filename;
                    }
                }

            }

            if (cursorFound >= 2)
                updated = false;

            // still fine? now assert: exactly one file with cursor and exactly one cursor in the file
            if (updated)
            {
                if (cursorFound == 0)
                {
                    Printer.PrepareConsole("Error");
                    Printer.Print("Cursor not found! (Cursor char is: ^| )", errorClr);
                    updated = false;
                }

                else // exactly one file
                {
                    string t = File.ReadAllText(currentFile);

                    MatchCollection matches = Regex.Matches(t, "\\^\\|");
                    if (matches.Count > 1)
                    {
                        Printer.PrepareConsole("Error");
                        Printer.Print(string.Format("More then one occurance of cursor found in current file:\n{0}",currentFile), errorClr);
                        updated = false;
                    }
                }

            }


            return updated;
        }

        private void DoIntelliSense()
        {
            string filePaths = string.Join(" ",
                di.GetFiles().ToList().ConvertAll(fi => fi.FullName));

            using (Process process = new Process())
            {
                string arguments = "";
                arguments += "-f " + "\"" + filePaths + "\"";
                arguments += " -provider " + currentLocation + "\\AssemblyReader.exe";
                process.StartInfo = new ProcessStartInfo
                {
                    FileName = intellisenseProg,
                    Arguments = arguments,
                    UseShellExecute = false,
                    RedirectStandardOutput = true
                };

                process.Start();

                StreamReader reader = process.StandardOutput;
                string output = reader.ReadToEnd();

                Printer.PrepareConsole("Done");
                Printer.PrintIntellisenseOutput(output);
                Console.SetCursorPosition(0, 0);

                process.WaitForExit();
            }

        }

    }
}
