/**
 * @file test_reader.c
 * @brief 媒体读取器测试
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "core/media_context.h"
#include "input/media_reader.h"
#include "utils/media_utils.h"

void test_reader_create_destroy(void)
{
    printf("Testing reader create/destroy...\n");
    
    MediaReader *reader = media_reader_create();
    assert(reader != NULL);
    
    MediaReaderState state = media_reader_get_state(reader);
    assert(state == MEDIA_READER_STATE_CLOSED);
    
    media_reader_destroy(&reader);
    assert(reader == NULL);
    
    printf("  Reader create/destroy tests passed!\n");
}

void test_reader_config(void)
{
    printf("Testing reader configuration...\n");
    
    MediaReaderConfig config;
    media_reader_config_default(&config);
    
    assert(config.buffer_size > 0);
    assert(config.probe_size > 0);
    assert(config.thread_count >= 1);
    
    MediaReader *reader = media_reader_create();
    assert(reader != NULL);
    
    MediaErrorCode ret = media_reader_set_config(reader, &config);
    assert(ret == MEDIA_SUCCESS);
    
    media_reader_destroy(&reader);
    
    printf("  Reader configuration tests passed!\n");
}

void test_reader_open_invalid(void)
{
    printf("Testing reader open with invalid file...\n");
    
    MediaReader *reader = media_reader_create();
    assert(reader != NULL);
    
    MediaErrorCode ret = media_reader_open(reader, "nonexistent_file.mp4");
    assert(ret == MEDIA_ERROR_FILE_NOT_FOUND);
    
    media_reader_destroy(&reader);
    
    printf("  Invalid file open tests passed!\n");
}

void test_reader_info(void)
{
    printf("Testing reader info functions...\n");
    
    MediaReader *reader = media_reader_create();
    assert(reader != NULL);
    
    /* 在没有打开文件时调用信息函数 */
    int video_idx = media_reader_get_video_stream_index(reader);
    assert(video_idx == -1);
    
    int audio_idx = media_reader_get_audio_stream_index(reader);
    assert(audio_idx == -1);
    
    double duration = media_reader_get_duration(reader);
    assert(duration == 0.0);
    
    media_reader_destroy(&reader);
    
    printf("  Reader info tests passed!\n");
}

void print_reader_info(MediaReader *reader)
{
    printf("\n  File Information:\n");
    printf("    Duration: %.2f seconds\n", media_reader_get_duration(reader));
    printf("    Bitrate: %d kbps\n", media_reader_get_bitrate(reader) / 1000);
    
    int video_idx = media_reader_get_video_stream_index(reader);
    if (video_idx >= 0) {
        printf("    Video Stream:\n");
        printf("      Index: %d\n", video_idx);
        printf("      Resolution: %dx%d\n", 
               media_reader_get_video_width(reader),
               media_reader_get_video_height(reader));
        printf("      Frame Rate: %.2f fps\n", 
               media_rational_to_double(media_reader_get_frame_rate(reader)));
    }
    
    int audio_idx = media_reader_get_audio_stream_index(reader);
    if (audio_idx >= 0) {
        printf("    Audio Stream:\n");
        printf("      Index: %d\n", audio_idx);
        printf("      Sample Rate: %d Hz\n", media_reader_get_sample_rate(reader));
        printf("      Channels: %d\n", media_reader_get_channels(reader));
    }
}

int main(int argc, char *argv[])
{
    printf("=== Media Reader Tests ===\n\n");
    
    /* 初始化媒体框架 */
    MediaErrorCode ret = media_init();
    if (ret != MEDIA_SUCCESS) {
        fprintf(stderr, "Failed to initialize media framework\n");
        return 1;
    }
    
    test_reader_create_destroy();
    test_reader_config();
    test_reader_open_invalid();
    test_reader_info();
    
    /* 如果提供了测试文件，进行实际读取测试 */
    if (argc > 1) {
        printf("\nTesting with file: %s\n", argv[1]);
        
        MediaReader *reader = media_reader_create();
        assert(reader != NULL);
        
        ret = media_reader_open(reader, argv[1]);
        if (ret == MEDIA_SUCCESS) {
            print_reader_info(reader);
            
            /* 读取几帧 */
            printf("\n  Reading frames...\n");
            int frame_count = 0;
            MediaFrame *frame = NULL;
            
            while (frame_count < 10) {
                ret = media_reader_read_frame(reader, &frame);
                if (ret == MEDIA_EOF) {
                    printf("    Reached EOF after %d frames\n", frame_count);
                    break;
                }
                if (ret != MEDIA_SUCCESS) {
                    printf("    Error reading frame: %d\n", ret);
                    break;
                }
                
                frame_count++;
                media_frame_free(&frame);
            }
            
            media_reader_close(reader);
        } else {
            printf("  Failed to open file: %d\n", ret);
        }
        
        media_reader_destroy(&reader);
    }
    
    /* 清理 */
    media_cleanup();
    
    printf("\n=== All tests passed! ===\n");
    return 0;
}
