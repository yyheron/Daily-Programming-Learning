using Avalonia.Controls;
using ReactiveUI;
using System.Collections.ObjectModel;
using System.Diagnostics;
using System.Reactive;
using System.Runtime.InteropServices;

namespace logReader.ViewModels
{
    public class MainWindowViewModel : ReactiveObject
    {
        public ObservableCollection<string> Products { get; } = new() { "LIDAR", "RADAR" };
        public ObservableCollection<string> Projects { get; } = new() { "binId11", "binId12" };

        private string _selectedProduct;
        public string SelectedProduct
        {
            get => _selectedProduct;
            set { this.RaiseAndSetIfChanged(ref _selectedProduct, value); this.RaisePropertyChanged(nameof(CanExecute)); }
        }

        private string _selectedProject;
        public string SelectedProject
        {
            get => _selectedProject;
            set { this.RaiseAndSetIfChanged(ref _selectedProject, value); this.RaisePropertyChanged(nameof(CanExecute)); }
        }

        private string _output;
        public string Output
        {
            get => _output;
            set => this.RaiseAndSetIfChanged(ref _output, value);
        }

        public bool CanExecute => !string.IsNullOrEmpty(SelectedProduct) && !string.IsNullOrEmpty(SelectedProject);

        public ReactiveCommand<Unit, Unit> MergeLogCommand { get; }
        public ReactiveCommand<Unit, Unit> BinToTextCommand { get; }

        public MainWindowViewModel()
        {
            MergeLogCommand = ReactiveCommand.Create(RunMergeLog);
            BinToTextCommand = ReactiveCommand.Create(RunBinToText);
        }

        private void RunMergeLog()
        {
            string script = RuntimeInformation.IsOSPlatform(OSPlatform.Windows) ? "bash" : "sh";
            string scriptPath = $"../scripts/log_merge.sh";
            string args = $"{SelectedProduct} {SelectedProject}";

            RunProcess(script, $"{scriptPath} {args}");
        }

        private void RunBinToText()
        {
            string exe = RuntimeInformation.IsOSPlatform(OSPlatform.Windows) ? "../recorder/BinToText.exe" : "../recorder/BinToText";
            string args = $"{SelectedProduct} {SelectedProject}";

            RunProcess(exe, args);
        }

        private void RunProcess(string fileName, string arguments)
        {
            try
            {
                var psi = new ProcessStartInfo
                {
                    FileName = fileName,
                    Arguments = arguments,
                    RedirectStandardOutput = true,
                    RedirectStandardError = true,
                    UseShellExecute = false,
                    CreateNoWindow = true,
                };
                using var process = Process.Start(psi);
                process.WaitForExit();
                Output = process.StandardOutput.ReadToEnd() + process.StandardError.ReadToEnd();
            }
            catch (System.Exception ex)
            {
                Output = $"运行失败: {ex.Message}";
            }
        }
    }
}
