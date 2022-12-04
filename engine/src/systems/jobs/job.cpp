
#include "job.h"

namespace C3D
{
	BaseJobInfo::BaseJobInfo(const JobType type, const JobPriority priority)
		: type(type), priority(priority)
	{}
}
