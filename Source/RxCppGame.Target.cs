using UnrealBuildTool;
using System.Collections.Generic;

public class RxCppGameTarget : TargetRules
{
	public RxCppGameTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.AddRange( new string[] { "RxCppGame" } );
	}
}
