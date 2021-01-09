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

        private const string PRIVATE = "PRIVATE";
        private const string PROTECTED = "PROTECTED";
        private const string PUBLIC = "PUBLIC";
        private const string STATIC = "STATIC";


        public List<Assembly> assemblies = new List<Assembly>();

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

            // ----- Find -----

            Type t = Type.GetType(invoker);
            if (t == null)
            {
                foreach (var asm in assemblies)
                {
                    t = asm.GetType(invoker);
                    if (t != null)
                        break;
                }

            }

            // ----- Print -----

            if (t != null)
            {
                var members = any ? t.GetMembers() : t.GetMember(member);
                foreach (var member in members)
                {
                    if ((member.MemberType & MemberTypes.Field) > 0)
                        PrintField(member as FieldInfo);
                    else if ((member.MemberType & MemberTypes.Property) > 0)
                        PrintProperty(member as PropertyInfo);
                    else if ((member.MemberType & MemberTypes.Method) > 0
                        || (member.MemberType & MemberTypes.Constructor) > 0)
                            PrintMethod(member as MethodBase);
                    else if ((member.MemberType & MemberTypes.TypeInfo) > 0
                        || (member.MemberType & MemberTypes.NestedType) > 0)
                            PrintType(member as TypeInfo);

                }
            }
        }

        public void PrintMethod(MethodBase mb)
        {
            Console.Write("METHOD" + " ");

            Console.Write(mb.ToString());

            if (mb.IsPublic) Console.Write(" " + PUBLIC);
            else if (mb.IsFamily) Console.Write(" " + PROTECTED);
            else if (mb.IsPrivate) Console.Write(" " + PRIVATE);

            if (mb.IsStatic) Console.Write(" " + STATIC);

            Console.Write('\n');

        }

        public void PrintType(TypeInfo ti)
        {
            Console.Write("TYPE" + " ");
            Console.Write(ti.ToString());
            Console.Write('\n');
        }

        public void PrintProperty(PropertyInfo pi)
        {
            Console.Write("VAR" + " ");
            Console.Write(pi.ToString());
            Console.Write('\n');
        }

        public void PrintField(FieldInfo fi)
        {
            Console.Write("VAR" + " ");

            Console.Write(fi.ToString());

            if (fi.IsPublic) Console.Write(" " + PUBLIC);
            else if (fi.IsFamily) Console.Write(" " + PROTECTED);
            else if (fi.IsPrivate) Console.Write(" " + PRIVATE);

            if (fi.IsStatic) Console.Write(" " + STATIC);

            Console.Write('\n');
        }
    }
}
