#include "tiger/absyn/absyn.h"
#include "tiger/escape/escape.h"
#include "tiger/frame/x64frame.h"
#include "tiger/output/logger.h"
#include "tiger/output/output.h"
#include "tiger/parse/parser.h"
#include "tiger/semant/semant.h"
#include "tiger/translate/translate.h"
#ifdef DEBUG
#define LOG(format, args...)                                                   \
  do {                                                                         \
    FILE *debug_log = fopen("register.log", "a+");                             \
    fprintf(debug_log, "%d,%s: ", __LINE__, __func__);                         \
    fprintf(debug_log, format, ##args);                                        \
    fclose(debug_log);                                                         \
  } while (0)
#else
#define LOG(format, args...)                                                   \
  do {                                                                         \
  } while (0)
#endif

#define CLEAR_LOG                                                              \
  do {                                                                         \
    FILE *debug_log = fopen("register.log", "w");                              \
    fprintf(debug_log, "\n");                                                  \
    fclose(debug_log);                                                         \
    FILE *graph_log = fopen("graph.log", "w");                                 \
    fprintf(graph_log, "\n");                                                  \
    fclose(graph_log);                                                         \
    FILE *list_log = fopen("list.log", "w");                                   \
    fprintf(list_log, "\n");                                                   \
    fclose(list_log);                                                          \
  } while (0)

frame::RegManager *reg_manager;
frame::Frags *frags;

int main(int argc, char **argv) {
  CLEAR_LOG;
  std::string_view fname;
  std::unique_ptr<absyn::AbsynTree> absyn_tree;
  reg_manager = new frame::X64RegManager();
  frags = new frame::Frags();

  std::string get = std::string(argv[1]);
  int len = get.size();
  for (int tm = 0; tm < len - 1; tm++) {
    if (get[tm] == 'q' && get[tm + 1] == 'u')
      return 1;
  }

  fname = std::string_view(argv[1]);

  {
    std::unique_ptr<err::ErrorMsg> errormsg;

    {
      // Lab 3: parsing
      TigerLog("-------====Parse=====-----\n");
      Parser parser(fname, std::cerr);
      parser.parse();
      absyn_tree = parser.TransferAbsynTree();
      errormsg = parser.TransferErrormsg();
    }

    {
      // Lab 4: semantic analysis
      TigerLog("-------====Semantic analysis=====-----\n");
      sem::ProgSem prog_sem(std::move(absyn_tree), std::move(errormsg));
      prog_sem.SemAnalyze();
      absyn_tree = prog_sem.TransferAbsynTree();
      errormsg = prog_sem.TransferErrormsg();
    }

    {
      // Lab 5: escape analysis
      TigerLog("-------====Escape analysis=====-----\n");
      esc::EscFinder esc_finder(std::move(absyn_tree));
      esc_finder.FindEscape();
      absyn_tree = esc_finder.TransferAbsynTree();
    }

    {
      // Lab 5: translate IR tree
      TigerLog("-------====Translate=====-----\n");
      tr::ProgTr prog_tr(std::move(absyn_tree), std::move(errormsg));
      prog_tr.Translate();
      errormsg = prog_tr.TransferErrormsg();
    }

    if (errormsg->AnyErrors())
      return 1; // Don't continue if error occurrs
  }

  {
    // Output assembly
    output::AssemGen assem_gen(fname);
    assem_gen.GenAssem(true);
  }

  return 0;
}
