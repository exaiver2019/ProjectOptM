// Project OptM - native launcher.
// Hosts the app script (ProjectOptM.ps1, embedded as a resource) inside this exe,
// so Windows sees a real application: its own icon, name, taskbar entry and admin prompt.
// Written for C# 5 so it builds with the compiler that ships with Windows.
using System;
using System.IO;
using System.Reflection;
using System.Text;
using System.Threading;
using System.Windows.Forms;

static class Program
{
    static MethodInfo FindMethod(Type t, string name, int paramCount, Type firstParam)
    {
        foreach (MethodInfo m in t.GetMethods(BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static))
        {
            if (m.Name != name || m.IsGenericMethodDefinition) continue;
            ParameterInfo[] p = m.GetParameters();
            if (p.Length != paramCount) continue;
            if (firstParam != null && p[0].ParameterType != firstParam) continue;
            return m;
        }
        throw new MissingMethodException(t.FullName, name);
    }

    [STAThread]
    static int Main(string[] args)
    {
        string self = Assembly.GetExecutingAssembly().Location;
        Environment.SetEnvironmentVariable("GO_SELF", self);

        string script;
        using (Stream s = Assembly.GetExecutingAssembly().GetManifestResourceStream("ProjectOptM.ps1"))
        using (StreamReader r = new StreamReader(s, Encoding.UTF8))
            script = r.ReadToEnd();

        try
        {
            // Windows PowerShell 5.1 engine (built into Windows 10/11), loaded at runtime
            Assembly sma = Assembly.Load("System.Management.Automation, Version=3.0.0.0, Culture=neutral, PublicKeyToken=31bf3856ad364e35");
            Type factory = sma.GetType("System.Management.Automation.Runspaces.RunspaceFactory", true);
            Type rsType = sma.GetType("System.Management.Automation.Runspaces.Runspace", true);
            Type optType = sma.GetType("System.Management.Automation.Runspaces.PSThreadOptions", true);
            Type psType = sma.GetType("System.Management.Automation.PowerShell", true);

            object rs = FindMethod(factory, "CreateRunspace", 0, null).Invoke(null, null);
            rsType.GetProperty("ApartmentState").SetValue(rs, ApartmentState.STA, null);        // WPF needs STA
            rsType.GetProperty("ThreadOptions").SetValue(rs, Enum.Parse(optType, "UseCurrentThread"), null);
            FindMethod(rsType, "Open", 0, null).Invoke(rs, null);

            object ps = FindMethod(psType, "Create", 0, null).Invoke(null, null);
            psType.GetProperty("Runspace").SetValue(ps, rs, null);
            FindMethod(psType, "AddScript", 1, typeof(string)).Invoke(ps, new object[] { script });
            FindMethod(psType, "Invoke", 0, null).Invoke(ps, null);

            try { FindMethod(rsType, "Close", 0, null).Invoke(rs, null); } catch { }
            return 0;
        }
        catch (Exception ex)
        {
            Exception e = ex;
            while (e is TargetInvocationException && e.InnerException != null) e = e.InnerException;
            MessageBox.Show("Project OptM couldn't start:\n\n" + e.Message, "Project OptM", MessageBoxButtons.OK, MessageBoxIcon.Error);
            return 1;
        }
    }
}
