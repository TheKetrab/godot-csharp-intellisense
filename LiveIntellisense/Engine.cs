﻿using System;
using System.IO;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading;
using System.Threading.Tasks;
using System.Diagnostics;

namespace LiveIntellisense
{
    class Engine
    {
        private ConsoleColor warningClr = ConsoleColor.Red;
        private ConsoleColor errorClr = ConsoleColor.DarkMagenta;

        public string cursorStr = "^|";

        private string inputDir;
        private int msTimeout;
        private string intellisenseProg;

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
                    Printer.PrepareConsole();
                    DoIntelliSense();
                }

                Thread.Sleep(msTimeout);
            }
        }

        private bool CheckForUpdates()
        {
            int cursorFound = 0;
            bool updated = false;
            foreach (var fi in di.GetFiles())
            {
                string text;

                // visible in dictionary?
                if (!files.ContainsKey(fi.FullName))
                {
                    updated = true; // added file
                    text = File.ReadAllText(fi.FullName);
                    files.Add(fi.FullName,text);
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

                // contains cursor?
                if (text.Contains(cursorStr))
                {
                    cursorFound++;
                    if (cursorFound > 1)
                    {
                        if (cursorFound == 2)
                        {
                            Printer.Print(string.Format(
                                "More than one cursor occurance found.\nFirst at: {0}", currentFile), errorClr);
                        }

                        Printer.Print(string.Format("> Cursor also found at: {0}", fi.FullName), warningClr);
                    }
                    else
                    {
                        currentFile = fi.FullName;
                    }
                }

            }

            if (cursorFound != 1)
                updated = false;

            return updated;
        }

        private void DoIntelliSense()
        {
            string filePaths = string.Join(" ",
                di.GetFiles().ToList().ConvertAll(fi => fi.FullName));

            using (Process process = new Process())
            {
                
                process.StartInfo = new ProcessStartInfo
                {
                    FileName = intellisenseProg,
                    Arguments = "\"" + filePaths + "\"",
                    UseShellExecute = false,
                    RedirectStandardOutput = true
                };

                process.Start();

                StreamReader reader = process.StandardOutput;
                string output = reader.ReadToEnd();

                Printer.PrintIntellisenseOutput(output);

                process.WaitForExit();
            }

        }



    }
}