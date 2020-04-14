/* Copyright JS Foundation and other contributors, http://js.foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecma-array-object.h"
#include "ecma-builtins.h"
#include "ecma-builtin-function-prototype.h"
#include "ecma-iterator-object.h"
#include "ecma-builtin-helpers.h"
#include "ecma-builtin-object.h"
#include "ecma-exceptions.h"
#include "ecma-function-object.h"
#include "ecma-gc.h"
#include "jcontext.h"

#if ENABLED (JERRY_ES2015_BUILTIN_REFLECT)

#define ECMA_BUILTINS_INTERNAL
#include "ecma-builtins-internal.h"

/**
 * This object has a custom dispatch function.
 */
#define BUILTIN_CUSTOM_DISPATCH

/**
 * List of built-in routine identifiers.
 */
enum
{
  ECMA_REFLECT_OBJECT_ROUTINE_START = ECMA_BUILTIN_ID__COUNT - 1,
  ECMA_REFLECT_OBJECT_GET, /* ECMA-262 v6, 26.1.6 */
  ECMA_REFLECT_OBJECT_SET, /* ECMA-262 v6, 26.1.13 */
  ECMA_REFLECT_OBJECT_HAS, /* ECMA-262 v6, 26.1.9 */
  ECMA_REFLECT_OBJECT_DELETE_PROPERTY, /* ECMA-262 v6, 26.1.4 */
  ECMA_REFLECT_OBJECT_CONSTRUCT, /* ECMA-262, 26.1.2 */
  ECMA_REFLECT_OBJECT_OWN_KEYS, /* ECMA-262 v6, 26.1.11 */
  ECMA_REFLECT_OBJECT_GET_PROTOTYPE_OF, /* ECMA-262 v6, 26.1.8 */
  ECMA_REFLECT_OBJECT_SET_PROTOTYPE_OF, /* ECMA-262 v6, 26.1.14 */
  ECMA_REFLECT_OBJECT_APPLY, /* ECMA-262 v6, 26.1.1 */
  ECMA_REFLECT_OBJECT_DEFINE_PROPERTY, /* ECMA-262 v6, 26.1.3 */
  ECMA_REFLECT_OBJECT_GET_OWN_PROPERTY_DESCRIPTOR, /* ECMA-262 v6, 26.1.7 */
  ECMA_REFLECT_OBJECT_IS_EXTENSIBLE, /* ECMA-262 v6, 26.1.10 */
  ECMA_REFLECT_OBJECT_PREVENT_EXTENSIONS, /* ECMA-262 v6, 26.1.12 */
};

#define BUILTIN_INC_HEADER_NAME "ecma-builtin-reflect.inc.h"
#define BUILTIN_UNDERSCORED_ID reflect
#include "ecma-builtin-internal-routines-template.inc.h"

/** \addtogroup ecma ECMA
 * @{
 *
 * \addtogroup ecmabuiltins
 * @{
 *
 * \addtogroup object ECMA Reflect object built-in
 * @{
 */

/**
 * Dispatcher for the built-in's routines.
 *
 * @return ecma value
 *         Returned value must be freed with ecma_free_value.
 */
ecma_value_t
ecma_builtin_reflect_dispatch_routine (uint16_t builtin_routine_id, /**< built-in wide routine
                                                                     *   identifier */
                                       ecma_value_t this_arg, /**< 'this' argument value */
                                       const ecma_value_t arguments_list[], /**< list of arguments
                                                                             *   passed to routine */
                                       ecma_length_t arguments_number) /**< length of arguments' list */
{
  JERRY_UNUSED (this_arg);
  JERRY_UNUSED (arguments_number);

  if (builtin_routine_id < ECMA_REFLECT_OBJECT_CONSTRUCT)
  {
    /* 1. */
    if (arguments_number == 0 || !ecma_is_value_object (arguments_list[0]))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("Argument is not an Object."));
    }

    /* 2. */
    ecma_string_t *name_str_p = ecma_op_to_prop_name (((arguments_number > 1) ? arguments_list[1]
                                                                              : ECMA_VALUE_UNDEFINED));

    /* 3. */
    if (name_str_p == NULL)
    {
      return ECMA_VALUE_ERROR;
    }

    ecma_value_t ret_value;
    ecma_object_t *target_p = ecma_get_object_from_value (arguments_list[0]);
    switch (builtin_routine_id)
    {
      case ECMA_REFLECT_OBJECT_GET:
      {
        ecma_value_t receiver = arguments_list[0];

        /* 4. */
        if (arguments_number > 2)
        {
          receiver = arguments_list[2];
        }

        ret_value = ecma_op_object_get_with_receiver (target_p, name_str_p, receiver);
        break;
      }

      case ECMA_REFLECT_OBJECT_HAS:
      {
        ret_value = ecma_op_object_has_property (target_p, name_str_p);
        break;
      }

      case ECMA_REFLECT_OBJECT_DELETE_PROPERTY:
      {
        ret_value = ecma_op_object_delete (target_p, name_str_p);
        break;
      }

      default:
      {
        JERRY_ASSERT (builtin_routine_id == ECMA_REFLECT_OBJECT_SET);

        ecma_value_t receiver = arguments_list[0];

        if (arguments_number > 3)
        {
          receiver = arguments_list[3];
        }

        ret_value = ecma_op_object_put_with_receiver (target_p, name_str_p, arguments_list[2], receiver);
        break;
      }
    }

    ecma_deref_ecma_string (name_str_p);
    return ret_value;
  }

  if (builtin_routine_id == ECMA_REFLECT_OBJECT_OWN_KEYS)
  {
    /* 1. */
    if (arguments_number == 0 || !ecma_is_value_object (arguments_list[0]))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("Argument is not an Object."));
    }

    ecma_object_t *target_p = ecma_get_object_from_value (arguments_list[0]);

    /* 2. 3. */
    return ecma_builtin_helper_object_get_properties (target_p, ECMA_LIST_SYMBOLS);
  }

  if (builtin_routine_id == ECMA_REFLECT_OBJECT_CONSTRUCT)
  {
    /* 1. */
    if (arguments_number < 1 || !ecma_is_constructor (arguments_list[0]))
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("Target is not a constructor"));
    }

    ecma_object_t *target_p = ecma_get_object_from_value (arguments_list[0]);

    /* 2. new_target_p is not supported for now*/
    ecma_object_t *new_target_p = target_p;

    if (arguments_number > 2)
    {
      /* 3. */
      if (!ecma_is_constructor (arguments_list[2]))
      {
        return ecma_raise_type_error (ECMA_ERR_MSG ("Target is not a constructor"));
      }

      new_target_p = ecma_get_object_from_value (arguments_list[2]);
    }

    /* 4. */
    if (arguments_number < 2)
    {
      return ecma_raise_type_error (ECMA_ERR_MSG ("Reflect.construct requires the second argument be an object"));
    }

    ecma_collection_t *coll_p = ecma_op_create_list_from_array_like (arguments_list[1]);

    if (coll_p == NULL)
    {
      return ECMA_VALUE_ERROR;
    }

    ecma_value_t ret_value = ecma_op_function_construct (target_p,
                                                         new_target_p,
                                                         coll_p->buffer_p,
                                                         coll_p->item_count);

    ecma_collection_free (coll_p);
    return ret_value;
  }

  if (!ecma_is_value_object (arguments_list[0]))
  {
    return ecma_raise_type_error (ECMA_ERR_MSG ("Argument is not an Object."));
  }

  switch (builtin_routine_id)
  {
    case ECMA_REFLECT_OBJECT_GET_PROTOTYPE_OF:
    {
      return ecma_builtin_object_object_get_prototype_of (ecma_get_object_from_value (arguments_list[0]));
    }
    case ECMA_REFLECT_OBJECT_SET_PROTOTYPE_OF:
    {
      ecma_value_t result = ecma_builtin_object_object_set_prototype_of (arguments_list[0], arguments_list[1]);
      bool is_error = ECMA_IS_VALUE_ERROR (result);

      if (is_error)
      {
        jcontext_release_exception ();
      }
      else
      {
        ecma_free_value (result);
      }

      return ecma_make_boolean_value (!is_error);
    }
    case ECMA_REFLECT_OBJECT_APPLY:
    {
      if (!ecma_op_is_callable (arguments_list[0]))
      {
        return ecma_raise_type_error (ECMA_ERR_MSG ("Argument 'this' is not a function."));
      }

      ecma_object_t *func_obj_p = ecma_get_object_from_value (arguments_list[0]);
      return ecma_builtin_function_prototype_object_apply (func_obj_p, arguments_list[1], arguments_list[2]);
    }
    case ECMA_REFLECT_OBJECT_DEFINE_PROPERTY:
    {
      ecma_object_t *obj_p = ecma_get_object_from_value (arguments_list[0]);
      ecma_string_t *name_str_p = ecma_op_to_prop_name (arguments_list[1]);

      if (name_str_p == NULL)
      {
        return ECMA_VALUE_ERROR;
      }

      ecma_value_t result = ecma_builtin_object_object_define_property (obj_p, name_str_p, arguments_list[2]);
      ecma_deref_ecma_string (name_str_p);
      bool is_error = ECMA_IS_VALUE_ERROR (result);

      if (is_error)
      {
        jcontext_release_exception ();
      }
      else
      {
        ecma_free_value (result);
      }

      return ecma_make_boolean_value (!is_error);
    }
    case ECMA_REFLECT_OBJECT_GET_OWN_PROPERTY_DESCRIPTOR:
    {
      ecma_object_t *obj_p = ecma_get_object_from_value (arguments_list[0]);
      ecma_string_t *name_str_p = ecma_op_to_prop_name (arguments_list[1]);

      if (name_str_p == NULL)
      {
        return ECMA_VALUE_ERROR;
      }

      ecma_value_t ret_val = ecma_builtin_object_object_get_own_property_descriptor (obj_p, name_str_p);
      ecma_deref_ecma_string (name_str_p);
      return ret_val;
    }
    case ECMA_REFLECT_OBJECT_IS_EXTENSIBLE:
    {
      ecma_object_t *obj_p = ecma_get_object_from_value (arguments_list[0]);
      return ecma_builtin_object_object_is_extensible (obj_p);
    }
    default:
    {
      JERRY_ASSERT (builtin_routine_id == ECMA_REFLECT_OBJECT_PREVENT_EXTENSIONS);
      ecma_object_t *obj_p = ecma_get_object_from_value (arguments_list[0]);
      return ecma_builtin_object_object_prevent_extensions (obj_p);
    }
  }
} /* ecma_builtin_reflect_dispatch_routine */

/**
 * @}
 * @}
 * @}
 */

#endif /* ENABLED (JERRY_ES2015_BUILTIN_REFLECT) */
