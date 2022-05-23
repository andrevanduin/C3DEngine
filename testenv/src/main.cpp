
#include <core/logger.h>

int main(int argc, char** argv)
{
	C3D::Logger::Init();
	C3D::Logger::PushPrefix("Rens");

	C3D::Logger::Info("Ik ben lekker rens! {}", 17);
	return 0;
}