#include <elf.h>
#include <fcntl.h>
#include <getopt.h>
#include <glog/logging.h>
#include <sys/mman.h>

#include <iomanip>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

#define LOG_KEY_VALUE(key, value) " " << key << "=" << value
#define LOG_KEY(key) LOG_KEY_VALUE(#key, key)
#define LOG_BITS(key) LOG_KEY_VALUE(#key, HexString(key))

std::string HexString(char* num, int length = -1) {
    if (length == -1) {
        length = 16;
    }
    std::stringstream ss;
    ss << "0x" << std::uppercase << std::setfill('0') << std::setw(length)
       << std::hex << (u_int64_t)num;
    return ss.str();
}

std::string HexString(const char* num, int length = -1) {
    if (length == -1) {
        length = 16;
    }
    std::stringstream ss;
    ss << "0x" << std::uppercase << std::setfill('0') << std::setw(length)
       << std::hex << (u_int64_t)num;
    return ss.str();
}

template <class T>
std::string HexString(T num, int length = -1) {
    if (length == -1) {
        length = sizeof(T) * 2;
    }
    std::stringstream ss;
    ss << "0x" << std::uppercase << std::setfill('0') << std::setw(length)
       << std::hex << +num;
    return ss.str();
}

class ELF {
   public:
    ELF(const std::string& filename, char* head, const size_t size)
        : filename_(filename), head_(head), size_(size) {
        ehdr_ = reinterpret_cast<Elf64_Ehdr*>(head);
        CHECK_EQ(ehdr()->e_type, ET_DYN);
        for (uint16_t i = 0; i < ehdr()->e_phnum; i++) {
            phdrs_.emplace_back(reinterpret_cast<Elf64_Phdr*>(
                head_ + ehdr()->e_phoff + i * ehdr()->e_phentsize));
        }
        for (auto ph : phdrs()) {
            LOG(INFO) << LOG_BITS(ph->p_type);
            if (ph->p_type == PT_DYNAMIC) {
                CHECK(ph_dynamic_ == NULL);
                ph_dynamic_ = ph;
            }
        }
    }

    void Show() {
        LOG(INFO) << "Ehdr:" << LOG_BITS(ehdr()->e_entry) << LOG_BITS(size_)
                  << LOG_BITS(head_);
        for (auto p : phdrs()) {
            LOG(INFO) << "Phdr:" << LOG_BITS(p->p_vaddr)
                      << LOG_BITS(p->p_offset) << LOG_BITS(p->p_filesz);
        }
    }

    std::string filename() { return filename_; }
    Elf64_Ehdr* ehdr() { return ehdr_; }
    char* head() { return head_; }
    std::vector<Elf64_Phdr*> phdrs() { return phdrs_; }

   private:
    char* head_;
    size_t size_;
    std::string filename_;
    Elf64_Ehdr* ehdr_;
    std::vector<Elf64_Phdr*> phdrs_;
    Elf64_Phdr* ph_dynamic_ = NULL;
    std::vector<Elf64_Dyn*> dyns_;
    std::vector<std::pair<char*, uint32_t>> memories_;
};

std::unique_ptr<ELF> ReadELF(const std::string& filename) {
    int fd = open(filename.c_str(), O_RDONLY);
    CHECK(fd >= 0) << "Failed to open " << filename;

    size_t size = lseek(fd, 0, SEEK_END);
    CHECK_GT(size, 8 + 16) << "Too small file: " << filename;

    size_t mapped_size = (size + 0xfff) & ~0xfff;

    char* p = (char*)mmap(NULL, mapped_size, PROT_READ | PROT_WRITE | PROT_EXEC,
                          MAP_PRIVATE, fd, 0);
    CHECK(p != MAP_FAILED) << "mmap failed: " << filename;

    return std::make_unique<ELF>(filename, p, mapped_size);
}

int main(int argc, char* argv[]) {
    google::InitGoogleLogging(argv[0]);

    static option long_options[] = {
        {"rewrite-mapping-file", required_argument, nullptr, 1},
        {"output", required_argument, nullptr, 'o'},
        {0, 0, 0, 0},
    };

    std::string input;
    std::string output;
    std::string rewrite_mapping_file;

    int opt;
    while ((opt = getopt_long(argc, argv, "l:", long_options, nullptr)) != -1) {
        switch (opt) {
            case 1:
                rewrite_mapping_file = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            default:
                CHECK(false);
                break;
        }
    }

    CHECK(optind + 1 == argc);
    input = argv[optind];
    if (output == "") {
        output = input + ".renamed";
    }

    auto main_binary = ReadELF(input);
    main_binary->Show();
}
