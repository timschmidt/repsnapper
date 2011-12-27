program clipper_demo;

uses
  Forms,
  main in 'main.pas' {MainForm},
  clipper in 'Clipper.pas',
  GR32_Misc in 'GR32_Misc.pas',
  GR32_PolygonsEx in 'GR32_PolygonsEx.pas',
  GR32_VPR in 'GR32_VPR.pas';

{$R *.res}

begin
  Application.Initialize;
  Application.CreateForm(TMainForm, MainForm);
  Application.Run;
end.
