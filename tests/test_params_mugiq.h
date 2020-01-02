/*
 * C. Kallidonis
 * This file follows the QUDA conventions for command-line options.
 * It serves as an extension in order to allow MUGIQ-related command-line options along with the QUDA ones.
 */

#include <array>
#include <externals/CLI11.hpp> // This is a QUDA include file
#include <test_params.h>  // This is a QUDA include file
#include <mugiq.h>

void add_eigen_option_mugiq(std::shared_ptr<QUDAApp> app);

//- External variables used in tests
extern MuGiqEigTask mugiq_eig_task;
extern MuGiqEigOperator mugiq_eig_operator;

/*
class MUGIQApp : public CLI::App
{
public:
  MUGIQApp(std::string app_description = "", std::string app_name = "") : CLI::App(app_description, app_name) {};
  
  virtual ~MUGIQApp() {};
};

std::shared_ptr<MUGIQApp> make_mugiq_app(std::string app_description = "MUGIQ test", std::string app_name = "");
void add_eigen_option_mugiq(std::shared_ptr<MUGIQApp> mugiq_app);
*/

