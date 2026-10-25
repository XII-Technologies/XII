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
				uint32 %v{3}
				p
				{
					string %MainRenderPipeline{"{ d83bb009-7ef8-315a-1f96-12e7df0c90f7 }"}
				}
			}
			o
			{
				Uuid %id{uint64{385040311378845408,1178138948935131612}}
				string %t{"xiiTextureAssetProfileConfig"}
				uint32 %v{2}
				p
				{
					uint16 %MaxResolution{16384}
					uint16 %RenderTargetResolution1{256}
					uint16 %RenderTargetResolution2{512}
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
					}
					string %Name{"Default"}
					string %TargetPlatform{"Windows"}
				}
			}
		}
	}
}
