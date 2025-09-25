#ifndef PDUR_CFG_H
#define PDUR_CFG_H


#ifdef __cplusplus
extern "C" {
#endif

/*! @note if define PDUR_ENABLE_DYNAMIC_MEMORY, the memory allocation in pdur library will use dynamic memory allocation,
            otherwise, it will use static memory allocation that is defined by pdur_config.h. */

#define PDUR_ENABLE_DYNAMIC_MEMORY

#ifdef __cplusplus
}
#endif



#endif /* PDUR_CFG_H */
