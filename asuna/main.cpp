#include "tracer/tracer.h"

#include "nvh/inputparser.h"

void help()
{
	LOGI("usage: asuna.exe [--help] [--out <path>] [--offline] [--scene <path>]\n\n");
	LOGI("   --scene   : input scene json (default: \"../scenes/dragon/scene.json\")\n");
	LOGI("   --out     : output file (default: \"asuna_out.hdr\")\n");
	LOGI("   --offline : disable gui and run offline\n");
}

int main(int argc, char **argv)
{
	// setup some basic things for the sample, logging file for example
	NVPSystem system(PROJECT_NAME);

	InputParser parser(argc, argv);
	if (parser.exist("--help"))
	{
		help();
		exit(0);
	}

	TracerInitState tis;
	if (parser.exist("--offline"))
		tis.m_offline = true;
	tis.m_outputname = parser.getString("--out", "asuna_out.hdr");
	tis.m_scenefile  = parser.getString("--scene", "../scenes/dragon/scene.json");

	Tracer asuna;
	asuna.init(tis);
	asuna.run();
	asuna.deinit();
	return 0;
}