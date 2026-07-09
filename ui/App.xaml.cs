using System.Windows;

namespace Executor;

public partial class App : Application
{
    protected override void OnStartup(StartupEventArgs e)
    {
        base.OnStartup(e);

        // Surface unhandled exceptions in the status bar rather than crashing.
        DispatcherUnhandledException += (_, ex) =>
        {
            MessageBox.Show(ex.Exception.Message, "Unhandled Error",
                            MessageBoxButton.OK, MessageBoxImage.Error);
            ex.Handled = true;
        };
    }
}
