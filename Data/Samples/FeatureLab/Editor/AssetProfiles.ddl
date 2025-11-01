AssetProfiles
{
	Config %Default
	{
		Objects
		{
			o
			{
				Uuid %id{uint64{10386675348308922682,207938457348376428}}
				string %t{"xiiRenderPipelineProfileConfig"}
				uint32 %v{1}
				p
				{
					VarDict %CameraPipelines{}
					string %DebugRenderPipeline{"{ fb641000-728c-436b-90b9-3144155d8eda }"}
					string %EditorRenderPipeline{"{ ec0ce8c4-0a42-4346-88af-8b3b23916770 }"}
					string %MainRenderPipeline{"{ b9f7ca83-90ba-43b2-8eec-b0efddb3f264 }"}
				}
			}
			o
			{
				Uuid %id{uint64{385040311378845408,1178138948935131612}}
				string %t{"xiiTextureAssetProfileConfig"}
				uint32 %v{1}
				p
				{
					uint16 %MaxResolution{16384}
				}
			}
			o
			{
				Uuid %id{uint64{6024007684197752254,9388485812360408817}}
				string %t{"xiiXRConfig"}
				uint32 %v{2}
				p
				{
					bool %EnableXR{false}
					string %XRRenderPipeline{"{ 2fe25ded-776c-7f9e-354f-e4c52a33d125 }"}
				}
			}
			o
			{
				Uuid %id{uint64{8341519292606584866,16089769571062246001}}
				string %t{"xiiPlatformProfile"}
				uint32 %v{1}
				string %n{"root"}
				p
				{
					VarArray %Configs
					{
						Uuid{uint64{10386675348308922682,207938457348376428}}
						Uuid{uint64{385040311378845408,1178138948935131612}}
						Uuid{uint64{6024007684197752254,9388485812360408817}}
					}
					string %Name{"Default"}
					string %TargetPlatform{"Windows"}
				}
			}
		}
	}
}
