#include <stdio.h>
#include <dirent.h>

static int db_files_sort(const struct dirent **a, const struct dirent **b) {
  static const char prefix[] = "d.";
  static const int prefix_len = sizeof(prefix) - 1;

  for (int i = 0; prefix[i]; i++) if (prefix[i] != a[0]->d_name[i]) return 0;
  for (int i = 0; prefix[i]; i++) if (prefix[i] != b[0]->d_name[i]) return 0;

  long a_ = atol(a[0]->d_name + prefix_len);
  long b_ = atol(b[0]->d_name + prefix_len);

  if (a_ == b_) return 0;
  if (a_ > b_) return 1;
  return -1;
}

static int db_files_select(const struct dirent *file) {
  static const char prefix[] = "d.";

  int i;
  for (i = 0; prefix[i]; i++) if (prefix[i] != file->d_name[i]) return 0;

  while (1) switch (file->d_name[i]) {
    case '\0': return 1;
    case '0' ... '9': i++;
      continue;
    default: return 0;
  }
}

int main (void) {
  struct dirent **eps;
  int n;

  n = scandir ("./", &eps, one, sort_it);
  if (n >= 0) {
    int cnt;
    for (cnt = 0; cnt < n; ++cnt) {
      puts (eps[cnt]->d_name);
      free(eps[cnt]);
    }
  } else
    perror ("Couldn't open the directory");

  free(eps);

  return 0;
}
