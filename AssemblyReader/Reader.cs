using System;
using System.Collections.Generic;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Text;
using System.Threading.Tasks;

namespace AssemblyReader
{
    class Reader
    {
        public bool any;
        public string member;
        public string invoker;

        public List<Assembly> assemblies;

        public Reader(string assdirs, string asspaths)
        {
            // load assemblies from directories0
            if (!string.IsNullOrEmpty(assdirs))
            {
                var splitted = assdirs.Split(' ');
                foreach (var dir in splitted)
                {
                    DirectoryInfo di = new DirectoryInfo(dir);
                    foreach (var fi in di.GetFiles())
                        LoadAssembly(fi);

                }                
            }

            // load directories from files
            if (!string.IsNullOrEmpty(asspaths))
            {
                var splitted = asspaths.Split(' ');
                foreach (var file in splitted)
                    LoadAssembly(new FileInfo(file));

            }
        }

        private bool LoadAssembly(FileInfo fi)
        {
            bool success = false;
            if (fi.Extension.Equals("dll", StringComparison.CurrentCultureIgnoreCase))
            {
                try
                {
                    Assembly asm = Assembly.LoadFrom(fi.FullName);
                    assemblies.Add(asm);
                    success = true;
                }
                catch (Exception) { }
            }

            return success;
        }

        public void Run()
        {
            Type t;
        }


    }
}
